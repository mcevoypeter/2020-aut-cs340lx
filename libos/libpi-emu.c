// libpi emulation.
#include "libos.h"
#include "rpi.h"

// needed by printk
int (*rpi_putchar)(int c) = sys_putchar;

// do nothing.
void uart_init(void) {}

// call exit.

void clean_reboot(void) {
    sys_exit(0);
}
