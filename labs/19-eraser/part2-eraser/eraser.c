/****************************************************************************
 * some starter code: you can ignore it if you like.
 */
#include "rpi.h"
#include "memtrace.h"
#include "eraser.h"
#include "eraser-internal.h"


static int cur_thread_id = 0;
static int eraser_level = 0;

#define MAXTHREADS 4

// current thread lockset: just assume a single lock.
static int *lockset[MAXTHREADS];

// should the empty lockset have an id?
static uint16_t lock_to_int(void *lock) {
#   define MAXLOCKS 32
    static void *locks[MAXLOCKS];

    // lock=<NULL> is always 0.
    if(!lock)
        return 0;
    for(unsigned i = 0; i < MAXLOCKS; i++) {
        if(locks[i] == lock)
            return i+1;
        if(!locks[i]) {
            locks[i] = lock;
            return i+1;
        }
    }
    panic("too many locks!!\n");
}

static uint32_t need_to_check = 0;
static void ldr_str_handler(int is_load_p, uint32_t pc, uint32_t addr) {
    state_t * shadow_word = (state_t *)((addr & ~0b11) + OneMB);
    trace("%s pc=%p addr=%p shadow_word=%p\n", is_load_p ? "ldr" : "str", pc, addr,
            shadow_word);
    // TODO: fix this ugly hack to get around memory setting issue in
    // memtrace_shadow
    if (shadow_word->state == SH_INVALID)
        shadow_word->state = SH_VIRGIN;
    switch (shadow_word->state) {
        case SH_VIRGIN:
            trace("addr=%p VIRGIN -> EXCLUSIVE\n", addr);
            shadow_word->state = SH_EXCLUSIVE;
            shadow_word->tid = cur_thread_id;
            break;
        case SH_SHARED:
            // refine lockset
            shadow_word->tid = cur_thread_id;
            shadow_word->ls = lock_to_int(lockset[cur_thread_id]);
            if (is_load_p) {
                trace("addr=%p staying in SHARED\n", addr);
                break;
            }
            shadow_word->state = SH_SHARED_MOD;
            trace("addr=%p SHARED -> SHARED_MOD\n");
            goto shared_mod;
        case SH_EXCLUSIVE:;
            if (shadow_word->tid == cur_thread_id) {
                trace("addr=%p staying in EXCLUSIVE\n", addr);
                break;
            } else if (is_load_p) {
                shadow_word->state = SH_SHARED;
                trace("addr=%p EXCLUSIVE -> SHARED\n", addr);
                break;
            }
            // set variable's lockset to current thread's lockset
            shadow_word->state = SH_SHARED_MOD;
            shadow_word->tid = cur_thread_id;
            shadow_word->ls = lock_to_int(lockset[cur_thread_id]);
            trace("addr=%p EXCLUSIVE -> SHARED_MOD\n", addr);

shared_mod: case SH_SHARED_MOD:
            if (shadow_word->ls == 0)
                trace_clean_exit(
                    "ERROR:lockset for thread=%u empty when %s addr=%p"
                    " in shared-modified state\n", cur_thread_id, 
                    is_load_p ? "reading" : "writing", addr
                );
            break;
        default:
            panic("invalid memory state=%u\n", shadow_word->state);
            break;
    }
}


// Tell eraser that [addr, addr+nbytes) is allocated (mark as 
// Virgin).
void eraser_mark_alloc(void *addr, unsigned nbytes) {
    memset(addr, SH_VIRGIN, nbytes);
}

// Tell eraser that [addr, addr+nbytes) is free (stop tracking).
// We don't try to catch use-after-free errors.
void eraser_mark_free(void *addr, unsigned nbytes) {
    memset(addr, SH_FREED, nbytes);
}

// we don't do anything at the moment...
void eraser_mark_lock(void *addr, unsigned nbytes) {
    return;
}

// indicate that we acquired/released lock <l>
// address must be unique.
void eraser_lock(void *l) {
    // assume one lock.
    assert(!lockset[cur_thread_id]);
    lockset[cur_thread_id] = l;
}

void eraser_unlock(void *l) {
    lockset[cur_thread_id] = 0;
}

// mark bytes [l, l+nbytes) as holding a lock.
void eraser_set_thread_id(int tid) {
    assert(tid);
    assert(tid < MAXTHREADS);
    cur_thread_id = tid;
}

static uint32_t eraser_initialized = 0;
static memtrace_t m;
int eraser_fn_level(int (*fn)(void), int level) {
    eraser_level = level;
    return eraser_check_fn(fn);
}

static uintptr_t heap_start, shadow_mem_start;
static void eraser_init(void) {
    m = memtrace_init(ldr_str_handler);

    // map heap 
    heap_start = 2*OneMB;
    memtrace_map(m, (void *)heap_start, OneMB);

    // map shadow memory
    if (need_to_check) {
        shadow_mem_start = heap_start + OneMB;
        memtrace_shadow(m, (void *)shadow_mem_start, OneMB);
    }

    eraser_initialized = 1;
}

// what do we have to do?   when we allocate, need to add to the mapping.
int eraser_check_fn(int (*fn)(void)) {
    need_to_check = 1;
    if (!eraser_initialized) {
        eraser_init();
    }
    return memtrace_fn(m, fn);
}

int eraser_trace_fn(int (*fn)(void)) {
    need_to_check = 0;
    if (!eraser_initialized) {
        eraser_init();
    }
    return memtrace_fn(m, fn);
}
