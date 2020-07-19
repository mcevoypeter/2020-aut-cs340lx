// engler: should have an error: first thread does not need a lock b/c is
// in exclusive state, but second accesses w/o a lock.
#include "eraser.h"

// has the lock/unlock etc implementation.
#include "fake-thread.h"

static int l;
int notmain_client() {
    assert(mode_is_user());

    eraser_set_thread_id(1);

    int *x = kmalloc(4);
    put32(x,1);   // should be fine.

    // "second" thread --- should be fine.
    eraser_set_thread_id(2);
    lock(&l);
    put32(x,0x12345678);   // should be fine.
    int v = get32(x);
    unlock(&l);

    trace("-----------------------------------------------------\n");
    trace("should have an error at pc=%p, addr=%p\n", get32, x); 
    // should have an error
    v = get32(x);

    return v;
}

void notmain() {
    assert(!mmu_is_enabled());
    
    int x = eraser_fn_level(notmain_client, ERASER_SHARED_EX);
    trace("x=%x\n", x);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
