#ifndef __MEMCHECK_H__
#define __MEMCHECK_H__
#include "rpi.h"
#include "rpi-constants.h"
#include "rpi-interrupts.h"
#include "mmu.h"
#include "last-fault.h"
#include "cpsr-util.h"


// one-time initialization
void memcheck_init(void);

// XXX: hack to test that we can resume.
void memcheck_continue_after_fault(void);

int memcheck_fn(int (*fn)(void));

// do not check, only trace <fn>: used for testing.
int memcheck_trace_only_fn(int (*fn)(void));


// for the moment, we just call these special allocator and free routines.
void *memcheck_alloc(unsigned n);
void memcheck_free(void *ptr);

void *sys_memcheck_alloc(unsigned n);
void sys_memcheck_free(void *ptr);



#endif
