#include "rpi.h"
#include "last-fault.h"

/**********************************************************************
 * helper code to track the last fault (used for testing).
 */
static last_fault_t last_fault;
static unsigned fault_count = 0;
last_fault_t last_fault_get(void) {
    return last_fault;
}
void last_fault_set(uint32_t fault_pc, uint32_t fault_addr, uint32_t reason) {
    last_fault = (last_fault_t) {
            .fault_addr = fault_addr,
            .fault_pc = fault_pc,
            .fault_reason = reason,
            .fault_count = ++fault_count
    };
}
void fault_expected(uint32_t pc, uint32_t addr, uint32_t reason, uint32_t fault_cnt) {
    last_fault_t l = last_fault_get();
    assert(l.fault_addr == addr);
    assert(l.fault_pc ==  pc);
    assert(l.fault_reason == reason);
    trace("saw expected fault at pc=%p, addr=%p, reason=%d", pc,addr,reason);
    if(fault_cnt) {
        assert(l.fault_count == fault_cnt);
        output(" fault_count=%d", fault_cnt);
    }
    output("\n");
}

