different levels of hello:
    0-hello is a normal hello that just runs int he same address space as PIX
    1-hello uses your libos: just calls putc
    2-hello uses your libos: calls printk
