#ifndef __PIX_CONSTANTS_H__
#define __PIX_CONSTANTS_H__
// put all the constants here as defines so that we can include in .S files if need be.

#define PHYS_OFFSET 0x80000000UL

// we keep it smaller just to make things a bit faster.
#define PHYS_MEM_SIZE (1024*1024*256)
// #define PHYS_MEM_SIZE (1024*1024*496)

#endif
