#include "rpi-asm.h"

@ arguments:
@   
MK_FN(sys_10_asm)
    push {r4-r12, lr}
    swi 10
    pop {r4-r12, lr}
    bx lr

