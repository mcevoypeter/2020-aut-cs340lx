#ifndef __COPROCESSOR_H__
#define __COPROCESSOR_H__

// flush prefetch buffer: 3-79
static inline void prefetch_flush(void) {
    uint32_t r = 0;
    asm volatile("mcr p15, 0, %[val], c7, c5, 4" :: [val] "r" (r));
}

// DIDR: 13-5, 13-26
static inline uint32_t cp14_didr_get(void) {
    uint32_t didr;
    asm volatile("mrc p14, 0, %[result], c0, c0, 0" : [result] "=r" (didr) ::);
    return didr;
}

// DSCR: 13-7, 13-26
static inline uint32_t cp14_dscr_get(void) {
    uint32_t dscr;
    asm volatile("mrc p14, 0, %[result], c0, c1, 0" : [result] "=r" (dscr) ::);
    return dscr;
}
static inline void cp14_dscr_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c1, 0" :: [val] "r" (r));
    prefetch_flush();
}

// WFAR: 13-12, 13-26
static inline uint32_t cp14_wfar_get(void) {
    uint32_t wfar;
    asm volatile("mrc p14, 0, %[result], c0, c6, 0" : [result] "=r" (wfar) ::);
    return wfar;
}

// WVR0: 13-20, 13-26
static inline uint32_t cp14_wvr0_get(void) { 
    uint32_t wvr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 6" : [result] "=r" (wvr0) ::);
    return wvr0;
}
static inline void cp14_wvr0_set(uint32_t r) { 
    asm volatile("mcr p14, 0, %[val], c0, c0, 6" :: [val] "r" (r));
    prefetch_flush();
}

// WCR0: 13-21, 13-26
static inline uint32_t cp14_wcr0_get(void) {
    uint32_t wcr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 7" : [result] "=r" (wcr0) ::);
    return wcr0;
}
static inline void cp14_wcr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 7" :: [val] "r" (r));
    prefetch_flush();
}

// BVR0: 13-16, 13-26
static inline uint32_t cp14_bvr0_get(void) { 
    uint32_t bvr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 4" : [result] "=r" (bvr0) ::);
    return bvr0;
}
static inline void cp14_bvr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 4" :: [val] "r" (r));
    prefetch_flush();
}

// BCR0: 13-17, 13-26
static inline uint32_t cp14_bcr0_get(void) {
    uint32_t bcr0;
    asm volatile("mrc p14, 0, %[result], c0, c0, 5" : [result] "=r" (bcr0) ::);
    return bcr0;
}
static inline void cp14_bcr0_set(uint32_t r) {
    asm volatile("mcr p14, 0, %[val], c0, c0, 5" :: [val] "r" (r));
    prefetch_flush();
}

// DFSR: 3-64
static inline uint32_t cp15_dfsr_get(void) {
    uint32_t dfsr;
    asm volatile("mrc p15, 0, %[result], c5, c0, 0" : [result] "=r" (dfsr) ::);
    return dfsr;
}

// 3-68: fault address register: hold the MVA that the fault occured at.
static inline uint32_t cp15_far_get(void) {
    uint32_t far;
    asm volatile("mrc p15, 0, %[result], c6, c0, 0" : [result] "=r" (far) ::); 
    return far;
}

// ISFR: 3-66 (hold source of last instruction)
static inline uint32_t cp15_ifsr_get(void) {
    uint32_t ifsr;
    asm volatile("mrc p15, 0, %[result], c5, c0, 1" : [result] "=r" (ifsr) ::);
    return ifsr;
}

// IFAR: 3-69 (hold address of function that caused prefetch fault)
static inline uint32_t cp15_ifar_get(void) {
    uint32_t ifar;
    asm volatile("mrc p15, 0, %[result], c6, c0, 1" : [result] "=r" (ifar) ::);
    return ifar;
}

// get the saved status register.
static inline uint32_t spsr_get(void) {
    uint32_t spsr;
    asm volatile("msr spsr, %[result]" : [result] "=r" (spsr) ::);
    return spsr;
}

// set the saved status register.
static inline void spsr_set(uint32_t spsr) {
    asm volatile("msr spsr, %[val]" :: [val] "r" (spsr));
    prefetch_flush();
}

#endif
