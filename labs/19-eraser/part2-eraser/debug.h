#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "bit-support.h"
#include "coprocessor.h"
#include "cpsr-util.h"


// enable the co-processor.
void debug_init(void);

// client supplied fault handler: give a pointer to the registers so 
// the client can modify them (for the moment pass NULL)
//  - <pc> is where the fault happened, 
//  - <addr> is the fault address.
typedef void (*handler_t)(uint32_t regs[16], uint32_t pc, uint32_t addr);

// set a watchpoint at <addr>: calls <handler> with a pointer to the registers.
void debug_watchpt0_on(uint32_t addr, handler_t watchpt_handler);
//void watchpt_set0(uint32_t addr, handler_t watchpt_handle);

// set a breakpoint at <addr>: call handler when the fault happens.
void debug_match_breakpt0_on(uint32_t addr, handler_t breakpt_handler);
//void brkpt_set0(uint32_t addr, handler_t brkpt_handler);

// set a mismatch on <addr> --- call <handler> on mismatch.
// NOTE:
//  - an easy way to mismatch the next instruction is to call with
//    use <addr>=0.
//  - you cannot get mismatches in "privileged" modes (all modes other than
//    USER_MODE)
//  - once you are in USER_MODE you cannot switch modes on your own since the 
//    the required "msr" instruction will be ignored.  if you do want to 
//    return from user space you'd have to do a system call ("swi") that switches.
void debug_mismatch_breakpt0_on(uint32_t addr, handler_t breakpt_handler);
//void brkpt_mismatch_set0(uint32_t addr, handler_t handler);

// disable mismatch breakpoint <addr>: error if doesn't exist.
uint32_t debug_breakpt0_off(uint32_t addr);
//void brkpt_mismatch_disable0(uint32_t addr);

// simple debug macro: can turn it off/on by calling <brk_verbose({0,1})>
#define brk_debug(args...) if(brk_verbose_p) debug(args)

extern int brk_verbose_p;
static inline void brk_verbose(int on_p) { brk_verbose_p = on_p; }

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
