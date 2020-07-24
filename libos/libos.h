#ifndef __LIBOS_H__
#define __LIBOS_H__

// get the system call numbers from pix.
#include "syscall-nums.h"

// use this to call system calls.  makes it easier to add different system calls.
// implement by doing a swi in assembly without modifying any registers.
long syscall(long sysno, ...);

void sys_exit(int code) __attribute__((noreturn));

// writes character to console.
int sys_putchar(int c);

#endif
