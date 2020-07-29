#include "libos.h"

void notmain(void) { 
    sys_clone();

    int pid = sys_getpid();
    printk("getpid=%d\n", pid);
    sys_exit(pid);
}
