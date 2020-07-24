#ifndef __LIBOS_ASM_H__
#define __LIBOS_ASM_H__

#include "rpi-asm.h"

// our initial hack for a stack (remember: grows down)
#define LIBOS_STACK         0x900000
#define LIBOS_CODE_START    0x800000

#endif
