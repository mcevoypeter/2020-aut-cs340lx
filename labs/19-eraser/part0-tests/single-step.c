#include "rpi.h"
#include "rpi-interrupts.h"
#include "single-step.h"
#include "cp14-debug.h"
#include "libc/helper-macros.h"
#include "sys-call-asm.h"

int single_step_init(void) {
    cp14_enable();
    return 1;
}

static unsigned step_cnt = 0;
static void single_step_handler(uint32_t regs[16], uint32_t pc, uint32_t addr) {
    step_cnt++; 
    brkpt_mismatch_set0(pc, single_step_handler);    
}

int single_step_run(int user_fn(void), uint32_t sp) {
    step_cnt = 0;

    brkpt_mismatch_set0(0, single_step_handler);

    int ret_val = user_mode_run_fn(user_fn, sp);

    brkpt_mismatch_disable0(0);

    return ret_val;
}

unsigned single_step_cnt(void) {
    return step_cnt;
}
