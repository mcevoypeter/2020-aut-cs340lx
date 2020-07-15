#include "rpi.h"
#include "debug.h"
#include "memcheck.h"
#include "memcheck-internal.h"

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
            memcheck_trap_disable();
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
            memcheck_trap_enable();
            return (int)ptr;
        case 2:;
            memcheck_trap_disable();
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
            memcheck_trap_enable();
        case 10: 
            return 10;
        default:
            panic("%u is an invalid syscall number\n");
    }
}
