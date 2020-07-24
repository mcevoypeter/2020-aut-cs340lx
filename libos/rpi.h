#ifndef __RPI_EMULATION_H__
#define __RPI_EMULATION_H__
// engler: libos emulation of rpi: we could include rpi.h directly, but for 
// clarity of what is implemented / what is not, we pull in routines one at a time.
// 
// we include both this file and the original rpi.h header so that we
// get warnings if there are conflicts in prototypes.
// 
// XXX: as we build more and more, is likely better to just include rpi.h and not
// duplicate.

// does nothing
void uart_init(void);

// calls exit(0)
void clean_reboot(void) __attribute__((noreturn));

// needed by printk
extern int (*rpi_putchar)(int c);

// entry point definition
void notmain(void);

// you should extend this as you pull in more and more routines

#endif
