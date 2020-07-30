#include "rpi.h"

// writes to a GPIO location: should fault.
void notmain(void) { 
    assert((uint32_t)notmain > (1024 * 1024 * 8));

    printk("main = %p\n", notmain);

    // GPIO
    unsigned bad_addr = 0x20000000;
    printk("going to read GPIO pointer=%p pc=%p\n", bad_addr, PUT32);
    GET32(bad_addr);
    printk("should not get here\n");
}
