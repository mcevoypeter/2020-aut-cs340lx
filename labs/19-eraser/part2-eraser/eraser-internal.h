#ifndef __ERASER_INTERNAL_H__
#define __ERASER_INTERNAL_H__

// note: the structure and states changed.
enum {
    SH_INVALID     = 1 << 0,
    SH_VIRGIN     = 1 << 1,      // writeable, allocated
    SH_FREED       = 1 << 2,
    SH_SHARED = 1 << 3,
    SH_EXLUSIVE   = 1 << 4,
    SH_SHARED_MOD      = 1 << 5,
};

typedef struct {
    unsigned char state;
    unsigned char tid;
    unsigned short ls;
} state_t;

_Static_assert(sizeof(state_t) == 4, "invalid size");

_Static_assert((SH_INVALID ^ SH_VIRGIN ^ SH_FREED ^ SH_SHARED
                ^ SH_EXLUSIVE ^ SH_SHARED_MOD) == 0b111111, 
                "illegal value: must not overlap");

static inline int sh_is_set(state_t v, unsigned state) {
    return (v.state & state) == state;
}
static inline int sh_is_virgin(state_t s) { return sh_is_set(s, SH_VIRGIN); }
static inline int sh_is_freed(state_t s) { return sh_is_set(s, SH_FREED); }

void *sys_memcheck_alloc(unsigned n);
void sys_memcheck_free(void *ptr);

#endif
