// engler: we do the trivial eraser, so this should give an error.
#include "eraser.h"

// has the lock/unlock etc implementation.
#include "fake-thread.h"

static int l;
int notmain_client() {
    assert(mode_is_user());

    eraser_set_thread_id(1);

    lock(&l);
    int *x = kmalloc(4);
    put32(x,0x12345678);   // should be fine.

    unlock(&l);
    trace("---------------------------------------------\n");
    trace("expect a store error at pc=%p, addr=%p\n", put32, x); 
    trace("---------------------------------------------\n");
    put32(x,4);         // error
    return 0;
}

void notmain() {
    assert(!mmu_is_enabled());
    
    int x = eraser_fn_level(notmain_client, ERASER_TRIVIAL);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
