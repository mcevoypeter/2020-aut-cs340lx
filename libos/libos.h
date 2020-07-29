#ifndef __LIBOS_H__
#define __LIBOS_H__

// get the system call numbers from pix.
#include "syscall-nums.h"
#include "rpi.h"

// use this to call system calls.  makes it easier to add different system calls.
// implement by doing a swi in assembly without modifying any registers.
long syscall(long sysno, ...);

void sys_exit(int code) __attribute__((noreturn));

// writes character to console.
int sys_putchar(int c);

int sys_getpid(void);
int sys_fork(void);
int sys_clone(void);

// probably should not do this, but for themoment.
int printk(const char *format, ...);

#endif
