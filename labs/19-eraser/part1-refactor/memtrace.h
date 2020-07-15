#ifndef __MEMTRACE_H__
#define __MEMTRACE_H__
/*
 * simple memory tracing.  we will extend this.
 *
 * example:
 *  memtrace_t m = memtrace_init(memcheck_handler);
 *  if(!memtrace_shadow(memcheck_h, shadow, OneMB))
 *      panic("shadow failed!\n");
 *  memtrace_fn(m, fn); 
 *
 * 
 * XXX: likely need lower-level routines that you can compose so that the 
 * interface is more flexible.  We hard-wire everything for simplicity.
 */

// type of the function to trace
typedef int (*memtrace_fn_t)(void);
// type of the handler that gets called on each load or store trap.
typedef void (*memtrace_handler_t)(int is_load_p, uint32_t pc, uint32_t addr);

// opaque handle to memtrace instance: used so you can have multiple instances 
// active (we don't use that today: you can just globally allocate what you
// need).
typedef struct memtrace *memtrace_t;

// initialize memtrace: call this first.
memtrace_t memtrace_init(memtrace_handler_t h);

// map [addr, addr+nbytes): memory that will cause faults unless gets remarked
// as shadow memory.
void memtrace_map(memtrace_t h, void *addr, uint32_t nbytes);

// mark as shadow memory: places anywhere if <addr>=0.
void *memtrace_shadow(memtrace_t m, void *addr, uint32_t nbytes);

// heavy-weight disable/enable trapping.
// public interface: takes a pointer to the handle in case we need it later.
void memtrace_trap_on(memtrace_t m);
void memtrace_trap_off(memtrace_t m);
int memtrace_trap_enabled(memtrace_t m);

// trace <fn>, calling <handler> on each load or store.  returns the 
// pointer to shadow memory (should the client allocate?)
int memtrace_fn(memtrace_t m, memtrace_fn_t fn);

#endif
