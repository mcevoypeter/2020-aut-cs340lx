// rpi mailbox interface.
//  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
//
//  a more reasonable but unofficial writeup:
//  http://magicsmoke.co.za/?p=284
//
#include "rpi.h"

enum { OneMB = 1024 * 1024 };

/***********************************************************************
 * mailbox interface
 */

#define MAILBOX_FULL   (1<<31)
#define MAILBOX_EMPTY  (1<<30)
#define MAILBOX_START  0x2000B880
#define GPU_MEM_OFFSET    0x40000000

// document states: only using 8 right now.
#define MBOX_CH  8

/*
    REGISTER	ADDRESS
    Read	        0x2000B880
    Poll	        0x2000B890
    Sender	        0x2000B894
    Status	        0x2000B898
    Configuration	0x2000B89C
    Write	        0x2000B8A0
 */
#define MBOX_READ   0x2000B880
#define MBOX_STATUS 0x2000B898
#define MBOX_WRITE  0x2000B8A0

// need to pass in the pointer as a GPU address?
static uint32_t uncached(volatile void *cp) { 
    // not sure this is needed.
	return (unsigned)cp | GPU_MEM_OFFSET; 	
}

void mbox_write(unsigned channel, volatile void *data) {
    assert(channel == MBOX_CH);
    // check that is 16-byte aligned
	assert((unsigned)data%16 == 0);

    // 1. we don't know what else we were doing before: sync up 
    //    memory.
    dev_barrier();

    // 2. if mbox STATUS is full, wait.
    while (GET32(MBOX_STATUS) & MAILBOX_FULL);

    // 3. write out the data along with the channel to WRITE
    PUT32(MBOX_WRITE, (uintptr_t)data | channel);

    // 4. make sure everything is flushed.
    dev_barrier();
}

unsigned mbox_read(unsigned channel) {
    assert(channel == MBOX_CH);

    // 1. probably don't need this since we call after mbox_write.
    dev_barrier();

    // 2. while mailbox is empty, wait.
	while(GET32(MBOX_STATUS) & MAILBOX_EMPTY);

    // 3. read from mailbox and check that the channel is set.
	unsigned v = GET32(MBOX_READ);

    // 4. verify that the reply is for <channel>
    if ((v & 0xf) != channel)
        panic("v=%u unintended for channel=%u\n", v, channel);

    // return it.
    return v;
}

unsigned mbox_send(unsigned channel, volatile void *data) {
    mbox_write(MBOX_CH, data);
    mbox_read(MBOX_CH);
    return 0;
}

/*
  Get board serial
    Tag: 0x00010004
    Request: Length: 0
    Response: Length: 8
    Value: u64: board serial
*/
int rpi_get_serial(unsigned *s) {
    static volatile unsigned *u = 0;

    // must be 16-byte aligned.
    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

    // should abstract this.
    u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // serial tag
    u[2] = 0x00010004;
    u[3] = 8;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // space for first 4 bytes of serial
    u[6] = 0;   // space for second 4 bytes of serial
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);

    if(u[1] != 0x80000000)
		panic("invalid response: got %x\n", u[1]);
    s[0] = u[5];
    s[1] = u[6];
    return 1;
}

uint32_t rpi_get_memsize(void) {
    static volatile unsigned *u = 0;

    if(!u)
        u = kmalloc_aligned(8*4,16);
    memset((void*)u, 0, 8*4);

    // should abstract this.
    u[0] = 8*4;   // total size in bytes.
    u[1] = 0;   // always 0 when sending.
    // ARM memory tag
    // (https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)
    u[2] = 0x00010005;
    u[3] = 8;   // response size size
    u[4] = 0;   // request size
    u[5] = 0;   // space for first 4 bytes of serial
    u[6] = 0;   // space for second 4 bytes of serial
    u[7] = 0;   // end tag

    mbox_send(MBOX_CH, u);

    if(u[1] != 0x80000000)
		panic("invalid response: got %x\n", u[1]);

    demand(u[5] == 0, "expected 0 base, have %d\n", u[5]);
    return u[6];
}

/*****************************************************************************
 * ATAG support.  r/pi does not do much.
 *
 * some writeups:
 *  - http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html
 *  - https://jsandler18.github.io/tutorial/wrangling-mem.html
 */
enum {
    ATAG_NONE = 0x00000000,
    ATAG_CORE = 0x54410001,
    ATAG_MEM = 0x54410002,
    ATAG_CMDLINE =	0x54410009,
};

uint32_t atag_mem(void) {
    for(uint32_t *atag = (void*)0x100; atag[1] != ATAG_NONE; atag += atag[0]) {
        if(atag[1] == ATAG_MEM) {
            demand(!atag[3], "base should be zero, is: %x", atag[3]);
            return atag[2];
        }
    }
    panic("did not find any memory size!\n");
}

void atag_print(void) {
    printk("----------------------------------------------------------\n");
    uint32_t size, *atag = (void*)0x100; 
    for(int i = 0; atag[1] != ATAG_NONE; atag += atag[0], i++) {
        printk("ATAG %d: val = %x, = size=%d\n\t", i, atag[1], atag[0]);

        switch(atag[1]) { 
        case ATAG_CORE: 
            printk("ATAG_CORE: flags=%d, pagesize=%d, rootdev=%d\n",  atag[2], atag[3], atag[4]);
            break;
        case ATAG_MEM: 
            size = atag[2];
            printk("ATAG_MEM: mem tag: %d bytes (%dMB)\n", size, size/(1024*1024));
            // start
            assert(!atag[3]);
            break;
        case ATAG_CMDLINE:
            printk("ATAG_CMDLINE: command line: <%s>\n", &atag[2]);
            break;
        default:
            panic("ATAG: unknown tag=%x\n", atag[1]);
            break;
        }
    }
    printk("----------------------------------------------------------\n");
}


/***********************************************************************
 * trivial driver.
 */
void notmain(void) { 
    atag_print();
    unsigned size = atag_mem();
    printk("atag mem = %d (%dMB)\n\n\n", size, size / OneMB);

    unsigned u[2];
    rpi_get_serial(u);
    printk("mailbox serial = 0x%x%x\n", u[0],u[1]);

    size = rpi_get_memsize();
    printk("mailbox physical mem: size=%d (%dMB)\n", size, size/OneMB);

#if 0
    // some other useful things.
    printk("board model = 0x%x\n", rpi_get_model());
    printk("board revision= 0x%x\n", rpi_get_revision());
#endif
    clean_reboot();
}
