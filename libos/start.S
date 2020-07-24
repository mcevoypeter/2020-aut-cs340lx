#include "rpi-asm.h"
#include "libos-asm.h"

// To keep this in the first portion of the binary.
.section ".text.boot"

#define prefetch_flush(reg) mov reg, #0; mcr p15, 0, r3, c7, c5, 4

.globl _start
_start:
    mov sp, #LIBOS_STACK
    bl _cstart
    bl sys_exit
