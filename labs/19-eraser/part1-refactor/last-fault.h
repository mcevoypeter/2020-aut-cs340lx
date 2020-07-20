#ifndef __LAST_FAULT_H__
#define __LAST_FAULT_H__


/**************************************************************
 * helper code: track information about the last fault.
 */

// a bit more general than memcheck, but we keep it here for simple.
typedef struct {
    uint32_t fault_addr,
             fault_pc,
             fault_reason,
             fault_count;
} last_fault_t;

// returns the last fault
last_fault_t last_fault_get(void);
// check that the last fault was at pc, using addr with <reason>.  
//  if <fault_cnt> not zero, checks that.
void fault_expected(uint32_t pc, uint32_t addr, uint32_t reason, uint32_t fault_cnt);

void last_fault_set(uint32_t fault_pc, uint32_t fault_addr, uint32_t reason);

#endif
