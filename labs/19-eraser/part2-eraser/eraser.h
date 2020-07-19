#ifndef __ERASER_H__
#define __ERASER_H__

// XXX: way too much stuff, cleanup.
#include "rpi.h"
#include "rpi-constants.h"
#include "rpi-interrupts.h"
#include "mmu.h"
#include "last-fault.h"
#include "cpsr-util.h"

// different levels of eraser.
enum { ERASER_TRIVIAL = 1, 
    ERASER_SHARED_EX = 2,  
    ERASER_SHARED = 3,  
    ERASER_HIGHEST = ERASER_SHARED
};

// Run eraser on <fn>
int eraser_check_fn(int (*fn)(void));

// user can pass in the level that we are checking at.
int eraser_fn_level(int (*fn)(void), int eraser_level);

// run but do not do tracking (helps for testing)
int eraser_trace_fn(int (*fn)(void));

// Tell eraser that [addr, addr+nbytes) is allocated (mark as 
// Virgin).
void eraser_mark_alloc(void *addr, unsigned nbytes);

// Tell eraser that [addr, addr+nbytes) is free (stop tracking).
// We don't try to catch use-after-free errors.
void eraser_mark_free(void *addr, unsigned nbytes);

// mark bytes [l, l+nbytes) as holding a lock.
void eraser_mark_lock(void *l, unsigned nbytes);

// indicate that we acquired/released lock <l>
void eraser_lock(void *l);
void eraser_unlock(void *l);

// indicate that we are running thread <tid>
void eraser_set_thread_id(int tid);

#endif
