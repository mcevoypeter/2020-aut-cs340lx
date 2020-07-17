// engler: does a simple store --- again, nothing should be different.
#include "eraser.h"
#include "cpsr-util.h"

int notmain_client() {
    assert(mode_is_user());
    // this should all work fine.
    int *x = kmalloc(4);
    put32(x, 0x12345678);
    return get32(x);
}

void notmain() {
    assert(!mmu_is_enabled());
    int x = eraser_trace_fn(notmain_client);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
