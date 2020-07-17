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
    put32(x,1);   // should be fine.

    unlock(&l);
    trace("should not have an error because a second thread does not touch %p\n", x);

    put32(x,0x12345678);   // should be fine.
    return get32(x);
}

void notmain() {
    assert(!mmu_is_enabled());
    
    int x = eraser_fn_level(notmain_client, ERASER_SHARED_EX);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
