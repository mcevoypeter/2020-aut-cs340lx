#ifndef __PIX_H__
#define __PIX_H__
#include "syscall-nums.h"

// use this to call system calls.  makes it easier to add different system calls.
// implement by doing a swi in assembly without modifying any registers.
long syscall(long sysno, ...);

// kill current process
void sys_exit(int code) ;

// writes character to console.
int sys_putchar(int c);


// used only for testing
void run_user_asm(void *);

#endif
