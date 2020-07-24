#include "rpi.h"
#include "bit-support.h"
#include "debug.h"
#include "cpsr-util.h"
#include "last-fault.h"
#include "memcheck-internal.h"
#include "memtrace.h"
#include "mmu.h"
#include "single-step.h"
#include "user-mode-asm.h"

// don't use dom id = 0 --- too easy to miss errors.
enum { dom_id = 1, track_id = 2, shadow_id = 3 };

// XXX: our page table, gross.
static fld_t *pt = 0;

// need some parameters for this.
static uintptr_t heap_start, shadow_mem_start;

// client-provided routine to be run on each ldr/str
static memtrace_handler_t load_store_handler;

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

static unsigned dom_perm_get(unsigned dom) {
    unsigned x = read_domain_access_ctrl();
    return bits_get(x, dom*2, (dom+1)*2);
}

// set the permission bits for domain id <dom> to <perm>
// leave the other domains the same.
static void dom_perm_set(unsigned dom, unsigned perm) {
    assert(dom < 16);
    assert(perm == DOM_client || perm == DOM_no_access);
    unsigned x = read_domain_access_ctrl();
    x = bits_set(x, dom*2, (dom+1)*2, perm);
    write_domain_access_ctrl(x);
}

static memtrace_t m;
memtrace_t memtrace_init(memtrace_handler_t h) {
    load_store_handler = h;

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
#if 0
    // map heap 
    heap_start = (uintptr_t)pt + OneMB;
    kmalloc_init_set_start(heap_start);
    mmu_map_section(pt, heap_start, heap_start, track_id);

    // map shadow memory
    shadow_mem_start = heap_start + OneMB;
    mmu_map_section(pt, shadow_mem_start, shadow_mem_start, shadow_id);
    memset((void *)shadow_mem_start, SH_INVALID, OneMB);
    dom_perm_set(shadow_id, DOM_no_access);
#endif

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
    
    return m;
}

void memtrace_map(memtrace_t h, void *addr, uint32_t nbytes) {
    uintptr_t a = (uintptr_t)addr;
    assert(a % OneMB == 0);
    assert(nbytes == OneMB);
    kmalloc_init_set_start(a);
    mmu_map_section(pt, a, a, track_id);
}

void *memtrace_shadow(memtrace_t m, void *addr, uint32_t nbytes) {
    // TODO: handle case where `addr == 0`, which is when we need to decide 
    // where to put shadow memory
    uintptr_t a = (uintptr_t)addr;
    assert(a % OneMB == 0);
    assert(nbytes == OneMB);
    mmu_map_section(pt, a, a, shadow_id);
    // TODO: set memory in a cleaner way so that it works for memcheck and
    // eraser
    for (uint32_t i = 0; i < OneMB/4; i++) 
        *((uint32_t *)addr + i) = SH_INVALID;
    //memset((void *)a, SH_INVALID, OneMB);
    dom_perm_set(shadow_id, DOM_no_access);
    return addr;
}

// turn memtracing on: right now, just enable the mmu (which should 

void memtrace_trap_on(memtrace_t m) {
    dom_perm_set(track_id, DOM_no_access);
    assert(memtrace_trap_enabled(m));
}

// disable traps on tracked memory: we still do permission based checks
// (you can disable this using DOM_manager).
void memtrace_trap_off(memtrace_t m) {
    dom_perm_set(track_id, DOM_client);
    assert(!memtrace_trap_enabled(m));
}

// is trapping enabled?
int memtrace_trap_enabled(memtrace_t m) {
    return dom_perm_get(track_id) == DOM_no_access;
}

// flush any needed caches).
static void memtrace_on(void) {
    debug("memtrace: about to turn ON\n");
    // note: this routine has to flush I/D cache and TLB, BTB, prefetch buffer.
    mmu_enable();
    assert(mmu_is_enabled());
    debug("memtrace: ON\n");
}

// turn memtrace off: right now just diable mmu (which should flush
// any needed caches)
static void memtrace_off(void) {
    // 6. disable.
    mmu_disable();
    assert(!mmu_is_enabled());
    debug("memtrace: OFF\n");
}

int memtrace_fn(memtrace_t m, memtrace_fn_t fn) {
    static int initialized_memtrace = 0;
    if (!initialized_memtrace) {
        single_step_init();
        initialized_memtrace = 1;
    }
             
    memtrace_on();
    assert(mmu_is_enabled());
    assert(mode_is_super());
    int ret_val = user_mode_run_fn(fn, STACK_ADDR2);
    assert(mmu_is_enabled());
    assert(mode_is_super());
    memtrace_off();
    assert(!mmu_is_enabled());
    return ret_val;
}

typedef enum {
    alignment = 0b1,
    tlb_miss = 0b0,
    alignment_deprecated = 0b11,
    operation_fault = 0b100,
    abort_on_first_translation = 0b1100,
    abort_on_second_translation = 0b1110,
    translation_section = 0b101,
    translation_page = 0b111,
    domain_section = 0b1001,
    domain_page = 0b1011,
    permission_section = 0b1101,
    permission_page = 0b1111,
    external_abort_precise = 0b1000,
    external_abort_deprecated = 0b1010,
    tlb_lock = 0b10100,
    coproc_data_abort = 0b11010,
    external_abort_imprecise = 0b10110,
    parity_error_exception = 0b11000,
    debug_event = 0b10
} reason_t;

static void single_step_handler(uint32_t regs[16], uint32_t pc, uint32_t addr) {
    //trace("got breakpoint mismatch at pc=%p, addr=%p\n", pc, addr);
    memtrace_trap_on(m);
    debug_breakpt0_off(pc-4);
}

// simple data abort handle: handle the different reasons for the tests.
void data_abort_vector(unsigned lr) {
    if(was_debug_datafault())
        mem_panic("should not have this\n");

    // b4-43
    unsigned dfsr = cp15_dfsr_get();
    unsigned fault_addr = cp15_far_get();

    // compute the reason.
    // b4-43: reason is bits [10,3:0] of dfsr.
    unsigned reason = bits_get(dfsr, 10, 10) | bits_get(dfsr, 0, 3);

    last_fault_set(lr, fault_addr, reason);

    switch (reason) {
        case domain_section: 
            //trace("got domain section fault for addr %p, at pc %p, for reason %b\n",
                    //fault_addr, lr, reason);
            memtrace_trap_off(m);
            dom_perm_set(shadow_id, DOM_client);
            uint32_t instr = *(uint32_t *)lr;
            int is_load_p = (instr >> 20) & 1;
            load_store_handler(is_load_p, lr, fault_addr);
            dom_perm_set(shadow_id, DOM_no_access);
            debug_mismatch_breakpt0_on(lr, single_step_handler);
            //debug_set_mismatch_breakpt0(lr, single_step_handler);
            break;
            // set single step mismatch
        case translation_section:
            // TODO: replace with trace_clean_exit
            trace("section xlate fault: addr=%p, at pc=%p\n", fault_addr, lr);
            clean_reboot();
        case permission_section:
            panic("section permission fault: %x", fault_addr);
        default: 
            panic("unkown reason %b\n", reason);
    }
    /*
         for SECTION_XLATE_FAULT:
            trace_clean_exit("section xlate fault: addr=%p, at pc=%p\n" 
         for DOMAIN_SECTION_FAULT:
            if(!mmu_resume_p)
                trace_clean_exit("Going to die!\n");
            else {
                trace("going to try to resume\n");
                memtrace_trap_off();
                return;
            }
        for SECTION_PERM_FAULT:
            panic("section permission fault: %x", fault_addr);

        otherwise
        default: panic("unknown reason %b\n", reason);
     */ 
}

// <pc> should point to the system call instruction.
//      can see the encoding on a3-29:  lower 24 bits hold the encoding.
// <r0> = the first argument passed to the system call.
// 
// system call numbers:
//  <1> - set spsr to the value of <r0>
int syscall_vector(unsigned pc, uint32_t r0, uint32_t r1, uint32_t r2) {
    // SWI: A4-210 of ARMv6 instruction manual
    uint32_t syscall_num = *(uint32_t *)pc & 0xffffff;
    switch (syscall_num) {
        case 0:;
            uint32_t spsr = r0;
            spsr_set(spsr);
            return 0;
        case 1:;
            uint32_t n = r0;
            memtrace_trap_off(m);
            uintptr_t ptr = (uintptr_t)kmalloc(n);
            //assert(heap_start <= ptr && ptr < heap_start + OneMB);


            trace("setting %u bytes of shadow memory starting at %p as %b\n", 
                    n, ptr+OneMB, SH_ALLOCED);

            // enable access to shadow memory
            dom_perm_set(shadow_id, DOM_client);
            assert(dom_perm_get(shadow_id) == DOM_client);

            // set shadow memory to alloced
            memset((char *)ptr+OneMB, SH_ALLOCED, n);

            // disable access to shadow memory
            dom_perm_set(shadow_id, DOM_no_access);
            assert(dom_perm_get(shadow_id) == DOM_no_access);
            memtrace_trap_on(m);
            return (int)ptr;
        case 2:;
            memtrace_trap_off(m);
            ptr = r0;
            kfree((void *)ptr);

            trace("setting %u bytes of shadow memory starting at %p as %b\n", 
                    4, ptr+OneMB, SH_ALLOCED);

            // enable access to shadow memory
            dom_perm_set(shadow_id, DOM_client);
            assert(dom_perm_get(shadow_id) == DOM_client);

            // set shadow memory to alloced
            memset((char *)ptr+OneMB, SH_FREED, 4);

            // disable access to shadow memory
            dom_perm_set(shadow_id, DOM_no_access);
            assert(dom_perm_get(shadow_id) == DOM_no_access);
            memtrace_trap_on(m);
        case 10: 
            return 10;
        default:
            panic("%u is an invalid syscall number\n");
    }
}
