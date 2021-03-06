// engler: should not have an error: we have shared state and no one does a write
// after the first thread.
#include "eraser.h"

// has the lock/unlock etc implementation.
#include "fake-thread.h"

static int l;
int notmain_client() {
    assert(mode_is_user());

    eraser_set_thread_id(1);

    // private: no errors.
    int *x = kmalloc(4);
    put32(x, 0x12345678);   // should be fine.

    // "second" thread --- read only
    eraser_set_thread_id(2);
    trace("should not have an error at pc=%p, addr=%p\n", get32, x); 

    // read only: fine.
    get32(x);
    get32(x);
    get32(x);
    get32(x);
    get32(x);
    get32(x);

    return get32(x);
}

void notmain() {
    assert(!mmu_is_enabled());
    
    int x = eraser_fn_level(notmain_client, ERASER_SHARED);
    trace("x=%x\n", x);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
