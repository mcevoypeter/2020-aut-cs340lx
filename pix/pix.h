#ifndef __PIX_H__
#define __PIX_H__
#include "syscall-nums.h"

/*
 * simple user process structure --- we're going to expand this, and probably
 * change significantly
 */

#include "rpi.h"

typedef struct first_level_descriptor pix_pt_t;

#define MAX_ENV 2
typedef struct env { 
    uint32_t regs[16];
    uint32_t cspsr;

    // all of these must point to virtual addresses in the pinnned
    // entry in the page table (which can never be deleted).
    
    // this pc does not need to be mapped since we jump to it in USER mode.
    // if it is not, they will get a datafault.
    // 
    // should have a debug mode that kills the app if it keeps getting data faults
    // without any intervening system calls.
    uint32_t init;              // initial code to jump to to start the process.

    // #define pix_new_pid(pid) (pid & 0xff0000) | (pid+1) & 0xffff)

    // next pid = (env_index << 24) | ((pid+1) & 0xffff) so there is never a 
    // repeat across processes.
    uint32_t pid;

    // address space id.  for now we keep it simple by killing off any asid on
    // exit.  can be more clever later.
    uint8_t asid;

    pix_pt_t *pt_pa;        // physical address
} env_t;

// use this to call system calls.  makes it easier to add different system calls.
// implement by doing a swi in assembly without modifying any registers.
long syscall(long sysno, ...);

// kill current process
void sys_exit(int code) ;

// writes character to console.
int sys_putchar(int c);

// returns pid of process
int sys_getpid(void);

// clone process. will jump to <pc> when it starts.
int sys_clone(uint32_t pc);


// used only for testing
void run_user_asm(void *);

// make/run the init process.
int run_init(unsigned *code, unsigned nbytes);

// internal routines: they do not belong here, but to make it easier than 
// hunting aroudn.
void schedule(void);

// current process structure
env_t *pix_cur_env(void);

// delete env <e>
void env_delete(env_t *e);

// return id for <e> --- should be [0..MAX_ENV)
unsigned  env_id(env_t *e);

#endif
