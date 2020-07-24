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
enum { dom1 = 1 };

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

void no_return(void) {
    panic("impossible: returned!\n");
}

static void map_addr_space(uint32_t user_code, int32_t dom_id) {
    // 1. init
    mmu_init();
    assert(!mmu_is_enabled());

    void *base = (void*)0x100000;

    pt = mmu_pt_alloc_at(base, 4096*4);
    assert(pt == base);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt, 0x0, 0x0, dom_id);
    // map the page table: for lab cksums must be at 0x100000.
    mmu_map_section(pt, 0x100000,  0x100000, dom_id);
    // map stack (grows down)
    mmu_map_section(pt, STACK_ADDR-OneMB, STACK_ADDR-OneMB, dom_id);
    // map user stack (grows down)
    mmu_map_section(pt, STACK_ADDR2-OneMB, STACK_ADDR2-OneMB, dom_id);
    // map user code
    mmu_map_section(pt, user_code, user_code, dom_id);

    // map the GPIO: make sure these are not cached and not writeback.
    // [how to check this in general?]
    mmu_map_section(pt, 0x20000000, 0x20000000, dom_id);
    mmu_map_section(pt, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt, 0x20200000, 0x20200000, dom_id);

    // if we don't setup the interrupt stack = super bad infinite loop
    mmu_map_section(pt, INT_STACK_ADDR-OneMB, INT_STACK_ADDR-OneMB, dom_id);

    // 3. install fault handler to catch if we make mistake.
    mmu_install_handlers();

    // 4. start the context switch:

    // set up permissions for the different domains: we only use <dom_id>
    // and permissions r/w.
    write_domain_access_ctrl(0b01 << dom_id*2);

    // use the sequence on B2-25
    set_procid_ttbr0(0x140e, dom_id, pt);

    // turn on mmu
    mmu_enable();
    assert(mmu_is_enabled());
}

static void run(unsigned *code, unsigned nbytes) {
    // copy code to target addr
    uint32_t dest_addr = code[0];
    memcpy((void *)dest_addr, code, nbytes);

    map_addr_space(dest_addr, dom1);

    user_mode_run(dest_addr + sizeof(uint32_t));
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
