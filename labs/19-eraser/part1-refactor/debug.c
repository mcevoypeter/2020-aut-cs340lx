// handle debug exceptions.
#include "rpi.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "debug.h"
#include "sys-lock.h"

int brk_verbose_p = 0;

void debug_init(void) {
    static int init_p = 0;

    if(!init_p) { 
        int_init();
        init_p = 1;
    }

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
    uint32_t dscr = cp14_dscr_get();
    dscr = bit_set(dscr, 15);  // set bit 15 to enable monitor debug mode
    dscr = bit_clr(dscr, 14);  // clear bit 14 to select monitor debug mode
    cp14_dscr_set(dscr);
    assert (cp14_dscr_get() == dscr);

    prefetch_flush();
    // assert(!cp14_watchpoint_occured());
}

// set the first watchpoint and call <handler> on debug fault: 13-47
static handler_t watchpt_handler0 = 0;
void debug_watchpt0_on(uint32_t addr, handler_t watchpt_handler) {
    // clear bit 0 of WCR to disable the watchpoint
    uint32_t wcr = cp14_wcr0_get();
    wcr = bit_clr(wcr, 0);
    cp14_wcr0_set(wcr); 

    // write the address to WVR
    cp14_wvr0_set(addr);
    assert(cp14_wvr0_get() == addr);
    
    // enable the watchpoint
    wcr = bit_clr(wcr, 20);             // set bit 20 of WCR to disable linking
    wcr = bits_set(wcr, 5, 8, 0b1111);  // set bits 5-8 of WCR to select addresses
    wcr = bits_set(wcr, 3, 4, 0b11);    // set bits 3-4 of WCR to allow loads and stores
    wcr = bits_set(wcr, 1, 2, 0b11);    // set bits 1-2 of WCR to allow user and privileged access
    wcr = bit_set(wcr, 0);              // set bit 0 of WCR to enable the watchpoint
    cp14_wcr0_set(wcr);
    assert(cp14_wcr0_get() == wcr);

    watchpt_handler0 = watchpt_handler;
}

// 13-16
static handler_t breakpt_handler0 = 0;
void debug_match_breakpt0_on(uint32_t addr, handler_t breakpt_handler) {
    uint32_t bcr = debug_breakpt0_off(addr);

    // write the address to BVR
    cp14_bvr0_set(addr);
    assert(cp14_bvr0_get() == addr);
    
    // enable the watchpoint
    bcr = bits_clr(bcr, 21, 22);            // set bits 21-22 of BCR to break on match
    bcr = bit_clr(bcr, 20);                 // set bit 20 of BCR to disable linking
    bcr = bits_clr(bcr, 14, 15);            // set bits 14-15 of BCR so breakpoint matches 
    bcr = bits_set(bcr, 5, 8, 0b1111);      // set bits 5-8 of BCR to select addresses
    bcr = bits_set(bcr, 1, 2, 0b11);        // set bits 1-2 of BCR to allow user and privileged access
    bcr = bit_set(bcr, 0);                  // set bit 0 of BCR to enable the watchpoint
    cp14_bcr0_set(bcr);
    assert(cp14_bcr0_get() == bcr);

    breakpt_handler0 = breakpt_handler;
}

void debug_mismatch_breakpt0_on(uint32_t addr, handler_t breakpt_handler) {
    uint32_t bcr = debug_breakpt0_off(addr);;

    // write the address to BVR
    cp14_bvr0_set(addr);
    assert(cp14_bvr0_get() == addr);
    
    // enable the watchpoint
    bcr = bits_set(bcr, 21, 22, 0b10);      // set bits 21-22 of BCR to break on match
    bcr = bit_clr(bcr, 20);                 // set bit 20 of BCR to disable linking
    bcr = bits_clr(bcr, 14, 15);            // set bits 14-15 of BCR so breakpoint matches 
    bcr = bits_set(bcr, 5, 8, 0b1111);      // set bits 5-8 of BCR to select addresses
    bcr = bits_set(bcr, 1, 2, 0b11);        // set bits 1-2 of BCR to allow user and privileged access
    bcr = bit_set(bcr, 0);                  // set bit 0 of BCR to enable the watchpoint
    cp14_bcr0_set(bcr);
    assert(cp14_bcr0_get() == bcr);

    breakpt_handler0 = breakpt_handler;
}

// should be this addr.
uint32_t debug_breakpt0_off(uint32_t addr) {
    // TODO: verify that the breakpoint is set on addr
    // clear bit 0 of BCR to disable the breakpoint
    uint32_t bcr = cp14_bcr0_get();
    bcr = bit_clr(bcr, 0);
    cp14_bcr0_set(bcr); 
    assert(cp14_bcr0_get() == bcr);
    return bcr;
}

// if get a breakpoint call <breakpt_handler0>
void prefetch_abort_vector(unsigned pc) {
    if(!was_debug_instfault())
        panic("impossible: should get no other faults\n");
    assert(breakpt_handler0);
    breakpt_handler0(0, pc, cp15_ifar_get());
}
