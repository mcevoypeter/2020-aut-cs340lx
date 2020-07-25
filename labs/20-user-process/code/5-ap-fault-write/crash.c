#include "rpi.h"

// writes to a GPIO location: should fault.
void notmain(void) { 
    assert((uint32_t)notmain > (1024 * 1024 * 8));

    printk("main = %p\n", notmain);

    // GPIO
    unsigned bad_addr = 0x20000000;
    printk("going to dereference GPIO pointer=%p pc=%p\n", bad_addr, PUT32);
    PUT32(bad_addr, 0);
    printk("should not get here\n");
}
