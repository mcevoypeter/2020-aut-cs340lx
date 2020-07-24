#include "pix.h"
#include "rpi.h"

#if 0
// you'll probably need some of this for mmu
#include "rpi-interrupts.h"
#include "rpi-constants.h"
#include "mmu.h"
#include "cpsr-util.h"
#endif

#include "init-hack.h"

// gross: for the moment we just allow system calls of max 3 arguments (will change)
typedef long (*syscall_t)();

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
    printk("syscall num = %d: a0=%x (a0='%c')\n", sysnum, a0,a0);
    syscall_t sys = syscalls[sysnum];
    assert(sys);
    assert(sysnum == SYS_PUTC);
    return sys(a0,a1,a2);
}

void no_return(void) {
    panic("impossible: returned!\n");
}


static void run(unsigned *code, unsigned nbytes) {
    uint32_t *dest_addr = (uint32_t *)code[0];
    memcpy(dest_addr, code, nbytes);
    BRANCHTO((uint32_t)(dest_addr+1));
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
