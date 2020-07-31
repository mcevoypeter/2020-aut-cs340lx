#include "pix.h"
#include "rpi.h"

// you'll probably need some of this for mmu
#include "init-hack.h"
#include "rpi-interrupts.h"
#include "rpi-constants.h"
#include "mmu.h"
#include "cpsr-util.h"
#include "user-mode-asm.h"

enum { OneMB = 1 << 20 };
enum {
    boot_dom = 0,
    kernel_dom = 1,
    user_dom = 2
};

// gross: for the moment we just allow system calls of max 3 arguments (will change)
typedef long (*syscall_t)();

static fld_t *pt;

static syscall_t syscalls[SYS_LAST] = {
    [SYS_EXIT] = (syscall_t)sys_exit,
    [SYS_PUTC] = (syscall_t)sys_putchar,
};

static int pix_cur_pid = 0;

void sys_exit(int code) {
    printk("%d: exiting with code <%d>\n", pix_cur_pid, code);
    clean_reboot();
}

int sys_putchar(int c) {
    return rpi_putchar(c);
}

long do_syscall(uint32_t sysnum, uint32_t a0, uint32_t a1, uint32_t a2) {
    if(sysnum >= SYS_LAST)
        return -1;
    printk("\tsyscall num = %d: a0=%x (a0='%c')\n", sysnum, a0,a0);
    syscall_t sys = syscalls[sysnum];
    assert(sys);
    assert(sysnum == SYS_EXIT || sysnum == SYS_PUTC);
    return sys(a0,a1,a2);
}

/****************************************************************
 * mechanical code for flipping permissions for the tracked memory
 * domain <track_id> on or off.
 *
   from in the armv6.h header:
    DOM_no_access = 0b00, // any access = fault.
    DOM_client = 0b01,      // client accesses check against permission bits in tlb
    DOM_reserved = 0b10,      // client accesses check against permission bits in tlb
    DOM_manager = 0b11,      // TLB access bits are ignored.
 */

static unsigned dom_perm_get(unsigned dom) {
    unsigned x = read_domain_access_ctrl();
    return bits_get(x, dom*2, (dom+1)*2);
}

// set the permission bits for domain id <dom> to <perm>
// leave the other domains the same.
static void dom_perm_set(unsigned dom, unsigned perm) {
    assert(dom < 16);
    assert(perm == DOM_client || perm == DOM_no_access);
    unsigned x = read_domain_access_ctrl();
    x = bits_set(x, dom*2, (dom+1)*2, perm);
    write_domain_access_ctrl(x);
}

void no_return(void) {
    panic("impossible: returned!\n");
}

typedef enum {
    no_access = 0b01,
    read_only = 0b10,
    read_write = 0b11
} user_perm_t;

static void set_user_perm(fld_t *p, user_perm_t perm) {
    pt = p;
    pt->APX = (perm >> 2) & 1;
    pt->AP = perm & 0b11;
}

void data_abort_vector(unsigned pc) {
    printk("data abort, pc=%p\n", pc);
    sys_exit(1);
}

// The first prefetch abort should come from the first instruction executed by
// the user process, at which point we mark the user domain as accessible and
// the kernel domain as inaccessible. 
void prefetch_abort_vector(unsigned pc) {
    static unsigned user_init = 0;
    if (!user_init) {
        printk("marking user space as accessible\n");
        dom_perm_set(user_dom, DOM_client);
        printk("1\n");
        mmu_mark_sec_no_access(pt, 0x20000000, 1);
        printk("2\n");
        mmu_mark_sec_no_access(pt, 0x20100000, 1);
        printk("3\n");
        //mmu_mark_sec_no_access(pt, 0x20200000, 1);
        printk("4\n");
        user_init = 1;
    }
}

static void map_addr_space(uint32_t *user_code) {
    // 1. init
    mmu_init();
    assert(!mmu_is_enabled());

    void *base = (void*)0x100000;

    pt = mmu_pt_alloc_at(base, 4096*4);
    assert(pt == base);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_global_section(pt, 0x0, 0x0, kernel_dom)->AP = no_access;
    // map the page table: for lab cksums must be at 0x100000.
    mmu_map_global_section(pt, 0x100000,  0x100000, kernel_dom)->AP = no_access;
    // map stack (grows down)
    mmu_map_global_section(pt, STACK_ADDR-OneMB, STACK_ADDR-OneMB, kernel_dom)->AP = read_only;
    // map user code
    mmu_map_private_section(pt, user_code[0], user_code[0], user_dom)->AP = read_write;

    // map the GPIO: make sure these are not cached and not writeback.
    // [how to check this in general?]
    mmu_map_global_section(pt, 0x20000000, 0x20000000, kernel_dom)->AP = no_access;
    mmu_map_global_section(pt, 0x20100000, 0x20100000, kernel_dom)->AP = no_access;
    mmu_map_global_section(pt, 0x20200000, 0x20200000, kernel_dom)->AP = no_access;

    // if we don't setup the interrupt stack = super bad infinite loop
    mmu_map_global_section(pt, INT_STACK_ADDR-OneMB, INT_STACK_ADDR-OneMB, kernel_dom)->AP = no_access;

    // 3. install fault handler to catch if we make mistake.
    mmu_install_handlers();

    // 4. start the context switch:

    // set up r/w permissions for the kernel and user domains 
    write_domain_access_ctrl(0b01 << kernel_dom*2 | 0b01 << user_dom*2);

    // use the sequence on B2-25
#   define FIRST_PID 0x140e
#   define FIRST_ASID 1
    set_procid_ttbr0(FIRST_PID, FIRST_ASID, pt);

    // turn on mmu
    mmu_enable();
    assert(mmu_is_enabled());
}

static void run(unsigned *code, unsigned nbytes) {
    // copy code to target addr
    uint32_t dest_addr = code[0];
    memcpy((void *)dest_addr, code, nbytes);

    map_addr_space((void *)dest_addr);

    // stick instructions from user_mode_run in user-mode-asm.S on the stack
    // to maintain access upon switching to user mode
    uint32_t sp[4] = {
        0xf1020010,
        0xe3a01000,
        0xee075f95,
        0xe12fff30
    };
    void (*user_mode_run_fp)(uint32_t) = (void *)sp;
    user_mode_run_fp(dest_addr+sizeof(uint32_t));
    no_return();
}

void notmain(void) {
    // TODO for today's lab:
    //  1. get the init process (is in the array in init-hack.h)
    //  2. setup mmu so can run the code --- should look like your other tools.
    //     just set up a 1MB process starting at the given linked address.
    //     (this is deliberately ridiculous so is easy to debug).
    //  3. run it (should look like other tools except does not return).
    printk("hello from pix\n");

    assert(code_init[0] == 0x800000);
    run(code_init, sizeof code_init);
}
