#ifndef __FAKE_THREAD_H__
#define __FAKE_THREAD_H__

/************************************************************************
 * this is what you would do to your posix implementation.
 */

static inline void lock_init(int *l) {
    eraser_mark_lock(&l, sizeof l);
}

static inline void lock(int *l) {
    eraser_lock(l);
}

static inline void unlock(int *l) {
    eraser_unlock(l);
}

static inline void *alloc(unsigned n) {
    void *ptr = kmalloc(n);
    eraser_mark_alloc(ptr, n);
    return ptr;
}
#endif
