#ifndef __DEBUG_H__
#define __DEBUG_H__
// handle debug exceptions.
#include "bit-support.h"
#include "coprocessor.h"

// client supplied fault handler: give a pointer to the registers so 
// the client can modify them (for the moment pass NULL)
//  - <pc> is where the fault happened, 
//  - <addr> is the fault address.
typedef void (*handler_t)(uint32_t regs[16], uint32_t pc, uint32_t addr);

void debug_init(void);

void debug_watchpt0_on(uint32_t addr, handler_t watchpt_handler);

void debug_match_breakpt0_on(uint32_t addr, handler_t breakpt_handler);

void debug_mismatch_breakpt0_on(uint32_t addr, handler_t breakpt_handler);

uint32_t debug_breakpt0_off(uint32_t addr);

// reason for watchpoint debug fault: 3-64 
static inline unsigned datafault_from_ld(void) {
    uint32_t dfsr = cp15_dfsr_get();
    // TODO: also check `was_debug_datafault`
    return !bit_isset(dfsr, 11);
}

static inline unsigned datafault_from_st(void) {
    return !datafault_from_ld();
}

// was this an instruction debug event fault?: 3-65
static inline unsigned was_debugfault_r(uint32_t r) {
    return !bit_isset(r, 10) && bits_get(r, 0, 3) == 0b10;
}

// was this a debug data fault?
static inline unsigned was_debug_datafault(void) {
    unsigned dfsr = cp15_dfsr_get();
    if(!was_debugfault_r(dfsr))
        return 0;
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
    uint32_t dscr = cp14_dscr_get();
    uint32_t source = bits_get(dscr, 2, 5);
    return source == 0b10;
}

// was this a debug instruction fault?
static inline unsigned was_debug_instfault(void) {
    uint32_t ifsr = cp15_ifsr_get();
    if(!was_debugfault_r(ifsr))
        panic("impossible: should only get datafaults\n");
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
    uint32_t dscr = cp14_dscr_get();
    uint32_t source = bits_get(dscr, 2, 5);
    return source == 0b1;
}
#endif
