// engler: doesn't do anything interesting: just checks that the basic dumb thing works.
#include "eraser.h"
#include "cpsr-util.h"

int notmain_client() {
    assert(mode_is_user());
    return 0x12345678;
}

void notmain() {
    assert(!mmu_is_enabled());
    int x = eraser_trace_only_fn(notmain_client);
    assert(x == 0x12345678);
    assert(!mmu_is_enabled());
    trace_clean_exit("success!!\n");
}
