// put all your system call invocations here.
#include "libos.h"

void sys_exit_helper(int code) {
    syscall(SYS_EXIT, code);
    // should not return.
}

// writes character to console.
int sys_putchar(int c) {
    return syscall(SYS_PUTC, c);
}
