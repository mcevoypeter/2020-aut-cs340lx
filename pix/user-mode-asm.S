#include "rpi-asm.h"

@ arguments:
@   r0 = function pointer
@   r1 = address of user stack
MK_FN(user_mode_run)
    cps #USER_MODE  @ set cpsr - switches to user mode
    mov r1, #0
    mcr p15, 0, r5, c7, c5, 4   @ prefetch flush
    blx r0
