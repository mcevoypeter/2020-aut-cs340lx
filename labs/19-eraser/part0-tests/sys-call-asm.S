#include "rpi-asm.h"

@ arguments:
@   r0 = function pointer
@   r1 = address of user stack
MK_FN(user_mode_run_fn)
    push {r2-r12, lr}
    mov r2, r0      @ move function pointer into r2
    mov r3, r1      @ move sp into r3
    mrs r4, cpsr    @ save super mode cpsr
    cps #USER_MODE  @ set cpsr - switches to user mode
    mov r5, #0
    mcr p15, 0, r5, c7, c5, 4   @ prefetch flush
    mov sp, r3      @ use separate stack for user mode

    blx r2          @ call routine
    mov r5, r0      @ save return value of routine
    mov r0, r4      @ give syscall handler the cpsr value to set
    swi 0           
    mov r0, r5      @ move return value into r0
    pop {r2-r12, lr}
    bx lr

@ arguments
@   r0 = n
MK_FN(sys_memcheck_alloc_trampoline)
    push {r4-r12, lr}
    swi 1
    pop {r4-r12, lr}
    bx lr

@ arguments
@   r0 = ptr
MK_FN(sys_memcheck_free_trampoline)
    push {r4-r12, lr}
    swi 2
    pop {r4-r12, lr}
    bx lr

@ arguments:
@   
MK_FN(sys_10_asm)
    push {r4-r12, lr}
    swi 10
    pop {r4-r12, lr}
    bx lr

