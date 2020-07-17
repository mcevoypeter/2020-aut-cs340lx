// engler: should not have an error: two threads, same lock, exclusive state.
#include "eraser.h"

// has the lock/unlock etc implementation.
#include "fake-thread.h"

static int l;
int notmain_client() {
    assert(mode_is_user());

    eraser_set_thread_id(1);

    lock(&l);
    int *x = kmalloc(4);
    put32(x,1);   // should be fine.

    unlock(&l);

    // "second" thread --- should get an error.
    eraser_set_thread_id(2);
    lock(&l);
    trace("should not have an error at pc=%p, addr=%p\n", put32, x); 
    put32(x,0x12345678);   // should be fine.
    int v = get32(x);
    unlock(&l);
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
