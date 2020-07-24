#include "syscall-nums.h"
#include "rpi-asm.h"

@ you might have to save less/more registers depending on your handler.
MK_FN(syscall)
    push {r4-r12, lr}
    swi 0
    pop {r4-r12, lr}
    bx lr

@ hack to get around the no return problem
MK_FN(sys_exit)
    b sys_exit_helper
