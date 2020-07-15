// engler: simple lock and unlock with one piece of shared state: should have no errors.
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

    int ret = get32(x);    // should be fine.
    unlock(&l);

    return ret;
}

void notmain() {
    assert(!mmu_is_enabled());
    int x = eraser_fn(notmain_client);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
