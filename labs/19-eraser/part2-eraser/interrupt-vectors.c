#include "rpi.h"
#include "debug.h"
#include "memcheck.h"
#include "memcheck-internal.h"
#include "memtrace.h"

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
    trace("got breakpoint mismatch at pc=%p, addr=%p\n", pc, addr);
    memtrace_trap_on();
    debug_breakpt0_off(pc-4);
}

// shouldn't happen: need to fix libpi so we don't have to do this.
void interrupt_vector(unsigned lr) {
    mmu_disable();
    panic("impossible\n");
}

// 0 if tracing, 1 if checking
unsigned shadow_check = 0;

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
            trace("got domain section fault for addr %p, at pc %p, for reason %b\n",
                    fault_addr, lr, reason);
            memtrace_trap_off();
            if (shadow_check) {
                memtrace_dom_perm_set(shadow_id, DOM_client);
                char *shadow_mem = (char *)fault_addr + OneMB;
                if (*shadow_mem == SH_FREED)
                    trace_clean_exit("ERROR:invalid use of freed memory at pc=%p, addr=%p\n",
                            lr, fault_addr);
                memtrace_dom_perm_set(shadow_id, DOM_no_access);
            }
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
    printk("system call: %u\n", syscall_num);
    switch (syscall_num) {
        case 0:;
            uint32_t spsr = r0;
            spsr_set(spsr);
            printk("done running function in user mode!\n");
            return 0;
        case 1:;
            uint32_t n = r0;
            memtrace_trap_off();
            uintptr_t ptr = (uintptr_t)kmalloc(n);
            //assert(heap_start <= ptr && ptr < heap_start + OneMB);


            trace("setting %u bytes of shadow memory starting at %p as %b\n", 
                    n, ptr+OneMB, SH_ALLOCED);

            // enable access to shadow memory
            memtrace_dom_perm_set(shadow_id, DOM_client);
            assert(memtrace_dom_perm_get(shadow_id) == DOM_client);

            // set shadow memory to alloced
            memset((char *)ptr+OneMB, SH_ALLOCED, n);

            // disable access to shadow memory
            memtrace_dom_perm_set(shadow_id, DOM_no_access);
            assert(memtrace_dom_perm_get(shadow_id) == DOM_no_access);
            memtrace_trap_on();
            return (int)ptr;
        case 2:;
            memtrace_trap_off();
            ptr = r0;
            kfree((void *)ptr);

            trace("setting %u bytes of shadow memory starting at %p as %b\n", 
                    4, ptr+OneMB, SH_ALLOCED);

            // enable access to shadow memory
            memtrace_dom_perm_set(shadow_id, DOM_client);
            assert(memtrace_dom_perm_get(shadow_id) == DOM_client);

            // set shadow memory to alloced
            memset((char *)ptr+OneMB, SH_FREED, 4);

            // disable access to shadow memory
            memtrace_dom_perm_set(shadow_id, DOM_no_access);
            assert(memtrace_dom_perm_get(shadow_id) == DOM_no_access);
            memtrace_trap_on();
        case 10: 
            return 10;
        default:
            panic("%u is an invalid syscall number\n");
    }
}
