#include "rpi.h"
#include "rpi-interrupts.h"
#include "single-step.h"
#include "libc/helper-macros.h"
#include "user-mode-asm.h"

void single_step_init(void) {
    debug_init();
}

static unsigned step_cnt = 0;
static void single_step_handler(uint32_t regs[16], uint32_t pc, uint32_t addr) {
    step_cnt++; 
    debug_mismatch_breakpt0_on(pc, single_step_handler);
}

int single_step_run(int user_fn(void), uint32_t sp) {
    step_cnt = 0;

    debug_mismatch_breakpt0_on(0, single_step_handler);

    int ret_val = user_mode_run_fn(user_fn, sp);

    debug_breakpt0_off(0);

    return ret_val;
}

unsigned single_step_cnt(void) {
    return step_cnt;
}
