#include "syscall-nums.h"
#include "rpi-asm.h"

@ you might have to save less/more registers depending on your handler.
MK_FN(syscall)
    push {lr}
    swi 0
    pop {lr}
    bx lr

@ hack to get around the no return problem
MK_FN(sys_exit)
    b sys_exit_helper
