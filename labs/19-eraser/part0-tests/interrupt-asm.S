/*
 * interrupt-asm.s
 *
 * Code for interrupt handling.  Refer to armisa.pdf in docs/ for what
 * the opcodes mean.
 */
#include "rpi-asm.h"

.globl _interrupt_table
.globl _interrupt_table_end
_interrupt_table:
  ldr pc, _reset_asm
  ldr pc, _undefined_instruction_asm
  ldr pc, _software_interrupt_asm
  ldr pc, _prefetch_abort_asm
  ldr pc, _data_abort_asm
  ldr pc, _reset_asm
  ldr pc, _interrupt_asm
fast_interrupt_asm:
  mov sp, #INT_STACK_ADDR 
  sub   lr, lr, #4
  bl    fast_interrupt_vector  

_reset_asm:                   .word reset_asm
_undefined_instruction_asm:   .word undefined_instruction_asm
_software_interrupt_asm:      .word software_interrupt_asm
_prefetch_abort_asm:          .word prefetch_abort_asm
_data_abort_asm:              .word data_abort_asm
_interrupt_asm:               .word interrupt_asm
_interrupt_table_end:


@ you need to look up in the arm pdf what the state of lr is for each.
@ sometimes you need to subtact 4, sometimes 8.
reset_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    reset_vector
undefined_instruction_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    undefined_instruction_vector
software_interrupt_asm:
  push {r4-r12, lr}
  mov r3, r2
  mov r2, r1
  mov r1, r0
  sub   r0, lr, #4
  bl    syscall_vector
  pop {r4-r12, lr}      @ don't pop r0 because we need the return value from syscall_vector
  movs pc, lr
prefetch_abort_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  push {r0-r12, lr}
  mov r0, lr
  bl    prefetch_abort_vector
  pop {r0-r12, lr}
  movs pc, lr
data_abort_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #8
  push {r0-r12, lr}
  mov r0, lr
  bl    data_abort_vector
  pop {r0-r12, lr}
  movs pc, lr
interrupt_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4
  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs
  mov   r0, lr              @ Pass old pc
  bl    irq_vector          @ C function
  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers
  @ return from interrupt handler: will re-enable general ints.
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.
