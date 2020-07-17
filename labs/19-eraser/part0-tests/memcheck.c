/*
 * engler: cs340lx
 *
 * start of a memchecker.  sort of lame in that you have to deal with a bunch of 
 * my starter code (sorry) --- the issue is that it might not be clear how to 
 * call the vm.
 *
 * implement:
 *  - static void memtrace_dom_perm_set(unsigned dom, unsigned perm) {
 *  - void data_abort_vector(unsigned lr) {
 * (1) is trivial, but just to remind you about domain id's.
 * (2) involves computing the fault reason and then acting.
 *
 */
#include "memcheck.h"
#include "memcheck-internal.h"
#include "memtrace.h"
#include "debug.h"
#include "libc/helper-macros.h"
#include "single-step.h"
#include "user-mode-asm.h"

void memcheck_init(void) {
    memtrace_init();
}

// hack to turn off resume / not resume for testing.
static unsigned mmu_resume_p = 0;
void memcheck_continue_after_fault(void) {
    mmu_resume_p = 1;
}

extern unsigned shadow_check;
int memcheck_fn(int (*fn)(void)) {
    shadow_check = 1;
    return memtrace_fn(fn);
}

// for the moment, we just call these special allocator and free routines.
void *memcheck_alloc(unsigned n) {
    return sys_memcheck_alloc(n);
}

void memcheck_free(void *ptr) {
    sys_memcheck_free(ptr);
}

void *sys_memcheck_alloc(unsigned n) {
    void *ptr;
    asm volatile("swi 1");
    asm volatile("mov %[result], r0" : [result] "=r" (ptr) ::);
    return ptr;
}

void sys_memcheck_free(void *ptr) {
    asm volatile("swi 2");
}
