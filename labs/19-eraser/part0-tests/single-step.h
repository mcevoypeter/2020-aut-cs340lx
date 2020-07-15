#ifndef __SINGLE_STEP_H__
#define __SINGLE_STEP_H__

#include "rpi.h"
#include "cp14-debug.h"
#include "bit-support.h"

// initialize single step mode: 1 on success.
int single_step_init(void);

// run a single routine <user_fn> in single step mode using stack <sp>
int single_step_run(int user_fn(void), uint32_t sp);

// return the number of instructions we single stepped.
//  (just count the faults)
unsigned single_step_cnt(void);

#endif
