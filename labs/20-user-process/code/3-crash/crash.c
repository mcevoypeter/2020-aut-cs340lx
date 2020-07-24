#include "rpi.h"

void notmain(void) { 
    // this should be unmapped
    unsigned bad_addr = 0x30000000;

    unsigned pc;
    asm volatile("mov %0, r15" : "=r"(pc));
    printk("about to do a read at pc=%x\n", pc);
    *(volatile unsigned *)0x30000000 = 0;
    printk("should not get here\n");

#if 0
    // this is GPIO: should reject r/w
    GET32(0x20000000);

    GET32(0x30000000);
    printk("going to try to write: %x (this should crash)!\n", gpio_addr); 
    PUT32(0x20000000, 0);
#endif
}
