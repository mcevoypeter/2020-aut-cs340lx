#ifndef __PIX_CONSTANTS_H__
#define __PIX_CONSTANTS_H__
// put all the constants here as defines so that we can include in .S files if need be.

// offset to add to a physical address to get to the aliased region.
#define PHYS_OFFSET 0x80000000UL

// we keep it smaller than 496 just to make things a bit faster.
#define MAX_SECS 256
#define PHYS_MEM_SIZE (1024*1024*MAX_SECS)

// maximum number of environments.
#define MAX_ENV 64

#endif
