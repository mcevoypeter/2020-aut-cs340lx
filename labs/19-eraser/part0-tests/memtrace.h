#ifndef __MEMTRACE_H__
#define __MEMTRACE_H__

void memtrace_init(void);

// enable mmu
void memtrace_on(void);

// disable mmu
void memtrace_off(void);

// enable all memory trapping -- currently assumes using a single domain id.
void memtrace_trap_on(void);

// disable all memory trapping -- currently assumes using a single domain id.
void memtrace_trap_off(void);

// is trapping enabled?
unsigned memtrace_trap_enabled(void);

int memtrace_fn(int (*fn)(void));

enum { OneMB = 1024 * 1024 };

// don't use dom id = 0 --- too easy to miss errors.
enum { dom_id = 1, track_id = 2, shadow_id = 3 };

unsigned memtrace_dom_perm_get(unsigned dom);

void memtrace_dom_perm_set(unsigned dom, unsigned perm);


#endif
