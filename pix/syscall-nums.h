#ifndef __SYSCALL_NUM_H__
#define __SYSCALL_NUM_H__

// keep these as defines so can include into assembly.
// each must be unique!
#define SYS_EXIT 2
#define SYS_PUTC 3
#define SYS_PUT_INT 4

#define SYS_GETPID 5
#define SYS_CLONE 6

#define SYS_LAST 7

#endif
