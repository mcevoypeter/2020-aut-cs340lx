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

static void check_ldr_str_handler(int is_load_p, uint32_t pc, uint32_t addr) {
    trace("%s addr=%p, pc=%p\n", is_load_p ? "ldr" : "str", addr, pc);
    char *shadow_mem = (char *)addr + OneMB;  
    if (*shadow_mem == SH_FREED) 
        trace_clean_exit("ERROR:invalid use of freed memory at pc=%p, addr=%p\n", 
                pc, addr);
}

int memcheck_fn(int (*fn)(void)) {
    memtrace_t m = memtrace_init(check_ldr_str_handler);
    return memtrace_fn(m, fn);
}

static void trace_ldr_str_handler(int is_load_p, uint32_t pc, uint32_t addr) {
    trace("%s addr=%p, pc=%p\n", is_load_p ? "ldr" : "str", addr, pc);
}

int memcheck_trace_only_fn(int (*fn)(void)) {
    memtrace_t m = memtrace_init(trace_ldr_str_handler);
    return memtrace_fn(m, fn);
}

static void *sys_memcheck_alloc(unsigned n) {
    void *ptr;
    asm volatile("swi 1");
    asm volatile("mov %[result], r0" : [result] "=r" (ptr) ::);
    return ptr;
}

// for the moment, we just call these special allocator and free routines.
void *memcheck_alloc(unsigned n) {
    return sys_memcheck_alloc(n);
}

static void sys_memcheck_free(void *ptr) {
    asm volatile("swi 2");
}

void memcheck_free(void *ptr) {
    sys_memcheck_free(ptr);
}


