#ifndef __USER_MODE_ASM_H__
#define __USER_MODE_ASM_H__

// assembly routine: run <fn> at user level with stack <sp>
//    XXX: visible here just so we can use it for testing.
int user_mode_run_fn(int (*fn)(void), uint32_t sp);

#endif
