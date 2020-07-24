Goal:   
 - run existing libpi programs at user space without any modification.

How:
 - make user-level versions of each of the libpi routines.  Oddly, these will sometimes
   have a surprising resemblance to your `libpi-fake` routines (e.g., `uart_init` will go 
   to an empty routine, `clean_reboot` will get mapped to `exit`).

A bunch of code (most of `libpi/libc`) can be imported directly.   We could also copy them
over so we can mess with them in ways that is not appriate for the kernel.  But for
now we just import.
