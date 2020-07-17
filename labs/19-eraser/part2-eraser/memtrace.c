#include "rpi.h"
#include "bit-support.h"
#include "coprocessor.h"
#include "debug.h"
#include "last-fault.h"
#include "memcheck-internal.h"
#include "memtrace.h"
#include "mmu.h"
#include "single-step.h"
#include "user-mode-asm.h"

// XXX: our page table, gross.
static fld_t *pt = 0;

// need some parameters for this.
static uintptr_t heap_start, shadow_mem_start;

void memtrace_init(void) {
    // 1. init
    mmu_init();
    assert(!mmu_is_enabled());

    void *base = (void*)0x100000;

    pt = mmu_pt_alloc_at(base, 4096*4);
    assert(pt == base);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt, 0x0, 0x0, dom_id);
    // map the page table: for lab cksums must be at 0x100000.
    mmu_map_section(pt, 0x100000,  0x100000, dom_id);
    // map stack (grows down)
    mmu_map_section(pt, STACK_ADDR-OneMB, STACK_ADDR-OneMB, dom_id);
    // map user stack (grows down)
    mmu_map_section(pt, STACK_ADDR2-OneMB, STACK_ADDR2-OneMB, dom_id);

    // map the GPIO: make sure these are not cached and not writeback.
    // [how to check this in general?]
    mmu_map_section(pt, 0x20000000, 0x20000000, dom_id);
    mmu_map_section(pt, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt, 0x20200000, 0x20200000, dom_id);

    // map heap 
    heap_start = (uintptr_t)pt + OneMB;
    kmalloc_init_set_start(heap_start);
    mmu_map_section(pt, heap_start, heap_start, track_id);

    // map shadow memory
    shadow_mem_start = heap_start + OneMB;
    mmu_map_section(pt, shadow_mem_start, shadow_mem_start, shadow_id);
    memset((void *)shadow_mem_start, SH_INVALID, OneMB);
    memtrace_dom_perm_set(shadow_id, DOM_no_access);

    // if we don't setup the interrupt stack = super bad infinite loop
    mmu_map_section(pt, INT_STACK_ADDR-OneMB, INT_STACK_ADDR-OneMB, dom_id);

    // 3. install fault handler to catch if we make mistake.
    mmu_install_handlers();

    // 4. start the context switch:

    // set up permissions for the different domains: we only use <dom_id>
    // and permissions r/w.
    write_domain_access_ctrl(0b01 << dom_id*2);

    // use the sequence on B2-25
    set_procid_ttbr0(0x140e, dom_id, pt);
}

// turn memtracing on: right now, just enable the mmu (which should 
// flush any needed caches).
void memtrace_on(void) {
    debug("memtrace: about to turn ON\n");
    // note: this routine has to flush I/D cache and TLB, BTB, prefetch buffer.
    mmu_enable();
    assert(mmu_is_enabled());
    debug("memtrace: ON\n");
}

// turn memtrace off: right now just diable mmu (which should flush
// any needed caches)
void memtrace_off(void) {
    // 6. disable.
    mmu_disable();
    assert(!mmu_is_enabled());
    debug("memtrace: OFF\n");
}

void memtrace_trap_on(void) {
    memtrace_dom_perm_set(track_id, DOM_no_access);
    assert(memtrace_trap_enabled());
}

// disable traps on tracked memory: we still do permission based checks
// (you can disable this using DOM_manager).
void memtrace_trap_off(void) {
    memtrace_dom_perm_set(track_id, DOM_client);
    assert(!memtrace_trap_enabled());
}

// is trapping enabled?
unsigned memtrace_trap_enabled(void) {
    return memtrace_dom_perm_get(track_id) == DOM_no_access;
}

extern unsigned shadow_check;
int memtrace_fn(int (*fn)(void)) {
    static int initialized_memtrace = 0;
    if (!initialized_memtrace) {
        memtrace_init();
        single_step_init();
        initialized_memtrace = 1;
    }
             
    memtrace_on();
    assert(mmu_is_enabled());
    assert(mode_is_super());
    trace("about to try running at user level\n");
    int ret_val = user_mode_run_fn(fn, STACK_ADDR2);
    assert(mmu_is_enabled());
    assert(mode_is_super());
    memtrace_off();
    assert(!mmu_is_enabled());
    return ret_val;
}

/****************************************************************
 * mechanical code for flipping permissions for the tracked memory
 * domain <track_id> on or off.
 *
   from in the armv6.h header:
    DOM_no_access = 0b00, // any access = fault.
    DOM_client = 0b01,      // client accesses check against permission bits in tlb
    DOM_reserved = 0b10,      // client accesses check against permission bits in tlb
    DOM_manager = 0b11,      // TLB access bits are ignored.
 */

unsigned memtrace_dom_perm_get(unsigned dom) {
    unsigned x = read_domain_access_ctrl();
    return bits_get(x, dom*2, (dom+1)*2);
}

// set the permission bits for domain id <dom> to <perm>
// leave the other domains the same.
void memtrace_dom_perm_set(unsigned dom, unsigned perm) {
    assert(dom < 16);
    assert(perm == DOM_client || perm == DOM_no_access);
    unsigned x = read_domain_access_ctrl();
    x = bits_set(x, dom*2, (dom+1)*2, perm);
    write_domain_access_ctrl(x);
}

