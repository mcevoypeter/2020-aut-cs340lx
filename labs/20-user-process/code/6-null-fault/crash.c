#include "rpi.h"

// outright crash.
void notmain(void) { 
    assert((uint32_t)notmain > (1024 * 1024 * 8));

    printk("main = %p\n", notmain);

    // GPIO
    unsigned bad_addr = 0x20000000;
    bad_addr = 4;

#if 0
    volatile unsigned pc;
    asm volatile("mov %0, r15" : "=r"(pc));

    printk("about to do a read at pc=%x\n", pc);
#endif
    printk("going to dereference low bad pointer=%p pc=%p\n", bad_addr, PUT32);
    PUT32(bad_addr, 0);
    printk("should not get here\n");
#if 0
    // this is GPIO: should reject r/w
    GET32(0x20000000);

    GET32(0x30000000);
    printk("going to try to write: %x (this should crash)!\n", gpio_addr); 
    PUT32(0x20000000, 0);
#endif
}
