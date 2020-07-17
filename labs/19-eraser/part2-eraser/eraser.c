/****************************************************************************
 * some starter code: you can ignore it if you like.
 */
#include "rpi.h"
#include "memtrace.h"
#include "eraser.h"
#include "eraser-internal.h"


static int cur_thread_id = 0;
static int eraser_level = 0;

#define MAXTHREADS 4

// current thread lockset: just assume a single lock.
static int *lockset[MAXTHREADS];

// Tell eraser that [addr, addr+nbytes) is allocated (mark as 
// Virgin).
void eraser_mark_alloc(void *addr, unsigned nbytes) {
    unimplemented();
}

// Tell eraser that [addr, addr+nbytes) is free (stop tracking).
// We don't try to catch use-after-free errors.
void eraser_mark_free(void *addr, unsigned nbytes) {
    unimplemented();
}

// we don't do anything at the moment...
void eraser_mark_lock(void *addr, unsigned nbytes) {
    return;
}

// indicate that we acquired/released lock <l>
// address must be unique.
void eraser_lock(void *l) {
    // assume one lock.
    assert(!lockset[cur_thread_id]);
    lockset[cur_thread_id] = l;
}

void eraser_unlock(void *l) {
    unimplemented();
}

// mark bytes [l, l+nbytes) as holding a lock.
void eraser_set_thread_id(int tid) {
    assert(tid);
    assert(tid < MAXTHREADS);
    unimplemented();
}

int eraser_fn_level(int (*fn)(void), int level) {
    eraser_level = level;
    return eraser_check_fn(fn);
}

// what do we have to do?   when we allocate, need to add to the mapping.
extern unsigned shadow_check;
int eraser_check_fn(int (*fn)(void)) {
    shadow_check = 1;
    return memtrace_fn(fn);
}

int eraser_trace_fn(int (*fn)(void)) {
    return memtrace_fn(fn);
}
