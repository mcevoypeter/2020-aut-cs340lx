## Eraser

Today we're going to build a simple version of eraser.  To keep it simple,
we will use fake threading calls that indicate:
  - What thread id is running.
  - When a context switch occurs.
  - When a lock and unlock occurs.
  - When you allocate and free memory.

We do fake threading to make it easier to develop your tool in just a
few hours.  Among other things, doing it fake rather than real makes
it easy to separate out issues where you get race condition in your
tool, makes deterministic debugging trivial, and also side-steps issues
arising from bugs that you might have in your context switching or thread
package library.

Don't fear:  Once the tool works on fake code we will then do pre-emptive
threads and retarget it.  Tentatively this is planned for next Tuesday
in order for us to first build a exact cross-checking tool (on Thursday)
that can record the read and write sets of different pieces of code and
their instruction traces, which can then be used to compare different
runs for equivalence.  This tool will make it easy to ruthlessly
test your context switching code in an easy way.  (And also virtual
memory hacking, speed hacks, interrupt changes, and all sorts of other
things.) As far as I know, no one has ever used the method we will build,
despite how much easier it makes developing OS code, so there is that too.
Looking forward to it!

Two checkoffs:
  1. Refactoring your code from last time and passing the tests.
  2. Adding an eraser tool (which should be fast given the refactoring) and 
     passing the tests.

--------------------------------------------------------------------------------------
#### Eraser review

Recall how the eraser code works:
   1. Initially each word is put in a virgin state.
   2. When the first thread uses the word, Eraser uses shadow memory to
      record that the word is in in the  "shared exclusive" state.
      (Hack to handle the case where a thread has allocated an object
      and is initializing it before releasing it to other threads).
   3. When another thread reads it, the word is put in a "shared state".  (Hack to handle the case
      where one thread initializes an object, but the object is then treated as a read-only
      object that thus does not need locking).
   4. When another thread writes it, the word is put in a "shared modified" state.  (I.e., this 
      is the standard notion of shared memory that needs locks to prevent data races as
       multiple threads read and write it).

In states 2-4, the lockset is refined (intersected) with the current set
of locks a thread holds.   Errors are only emitted if the lockset is empty
and the word is in the shared-modified state or is transitioning to it.

As usual we will build up in a simple way.  
  1. Just refine locks and give a message if the lockset goes empty.
  2. Then add the shared exclusive state to handle initialization.
  3. Then add the shared state to handle read-only objects.
  4. If you can, also write some more tests for us to use!

--------------------------------------------------------------------------------------
#### Refactoring.

As a first step, we'll refactor your code some so that the core tool
handles the single-stepping and memory tracing common to all tools we will
build and a simple extension "skin" can be used to extend the base tool
in a domain specific way (e.g., to do memory checking or race checking
or whatever).  Obviously it would have been better to just build it "the
right way" from the beginning, but all these labs are getting written
from scratch, so sometimes its not obvious what to do initially.  (And,
just to add insult to injury, our interfaces will likely change somewhat
as we build different tools over the quarter, so I apologize for that.)

We'll do this in two parts. 
   1. Modify your code so that it passes the checks that are in `part1-refactor/tests`
      (I should have used this type of testing from the beginning --- apologies for that 
      too.)
   2. Then rewrite the code and make sure you still pass them.  You'll make these tests
      much more fussy by first doing `make emitall` to emit all tests for your system
      and then doing `make checkexact` after each modification which will check that every
      line of output is the same.  This style of checking makes it easier to verify that
      you have correctly implemented code changes that should lead to no observable difference
      in behavior (e.g., as with refactoring, cleanup, speed changes, etc).  As discussed
      we will build a much more aggressive (and counter-intuitively easier to use) variant
      of this on Thursday but for now we just do things manually.

###### part0-tests: Initial code modifications for testing.

Modifications:
   1. I pulled the "last fault" stuff into its own file so that it can be
      adapted without conflict.  It is checked into `libpi` so you only need
      to delete it from your code.
   2. I changed `memcheck.h` to eliminate the last fault stuff, so you'll need the
      new one (or migrate your changes over if you modified it).
   3. I added a routine  `memcheck_trace_only_fn` that indicates we should not do shadow checking.
      (so that the earlier tests give the same results).  You should just set a flag in it
      and then call `memcheck_fn` and do nothing in the exception handler if the falg is set.
   4. Pull over the rest of your code from last lab into this directory.
   5. Make sure the tests using `make checkall` pass.
   6. Use `trace_panic` instead of `panic` so line numbers don't matter.
   7. Add some empty lines at the top of your memcheck.c (to make sure line numbers change)
      and make sure `make checkall` still passes.

NOTE: The checks should internally make sense even if you don't get exactly the same thing I did.
(It's fine to get different output.)

        // hack for testing.
        int memcheck_trace_only_fn(int (*fn)(void)) {
            memtrace_p = 1;
            return memcheck_fn(fn);
        }

###### part1-refactoring: Refactoring 

For this part we'll pull out most of the code that does single-stepping
and memory faults from `memcheck.c` and put it in `memtrace.c`, which
will just call your `memcheck` extension.  
  0. Run `make emitall` to record exactly what you code does for each test.
     Run `make checkall` to sanity check that your code works deterministically.
  1. Just look in `memtrace.h` and start implementing the routine.  Note: the 
     interface is way too simple (and read-only) at the moment; we'll likely
     add more routines in the future.
  2. After each modification, make sure `make checkall` still works.

**Note: our address space management is still a mess.  We're going to
clean this up as we figure out better what makes sense, but for the
moment just leave it the way it is.**

Simple renaming:
 - `memcheck_trap_enable` as `memtrace_trap_on`
 - `memcheck_trap_disable` as `memtrace_trap_off`
 - `memcheck_trap_enabled` as `memtrace_trap_enabled`

I would start simple:
  1. Modifying one routine at a time all within the same file.
  2. Then start moving all the memcheck stuff to the end of the file and 
     the trace at the top.
  3. When entirely done and pass all tests, split it.

Overall this is pretty simple, and I probably should have had you do it in a
prelab, but here we are.

To make things easier I've broken things down into pieces that add each
Eraser state incrementatlly.  However, you are welcome to just build
the full eraser from the beginning.  Note a common confusion  (my fault):
  - Each piece of the lab eliminates a class of false-positives, and
    when you re-run previous tests they may give different results.

--------------------------------------------------------------------------------------
#### Part 2: Useless Eraser

As a first pass we make a useless version of eraser that always gives
an error if any modification to heap data to an empty lockset.  This is
pretty useless --- among other reasons it gives errors for private data
that no other thread touches --- so we quickly refine it (in Part 3).

In terms of the Eraser paper's state machine, there are two states:
  1. Virgin (as in the paper) which transitions to Shared-modified on
     any access to a 32-bit word (read or write).
  2. Shared-modified: gives an error if the lockset for a given 
     32-bit word is empty (whether or not another thread can access the memory).

For simplicity, assume:
  1. We track exactly one piece of memory.
  2. We have at most one lock.
  3. We have exactly two threads.

Because of these trivializations you can use a few statically declared
global variables, no memory allocation, no set operations, no tricky
data structures.  You will just give an error if they touch the one
location in the heap without a lock.  Again, this would be a pretty
useless checker for an end-user, we do things this way so we can easily
debug your shadow memory and helper routines.

The header files and tests are in `part2-eraser`.
  - `eraser.h` gives the public interface.  You should implement these so the
    tests pass.
  - `eraser-internal.h` gives some possible internal definitions.  (You don't 
    have to use these.)  
  - `tests/fake-thread.h` is a fake thread "implementation" that calls into 
    your eraser tool using the routines defined in `eraser.h`.  The tests use
    this header; things should "just work".

Note:
  1. As in Eraser, we track things at a word level (rather than byte).
  2. The `state_t` structure defined in `eraser-internal.h` is the size of a word (4-bytes).
  3. Thus, for your shadow memory, you will allocate a region the same size as your heap.
  4. Given given an address `addr`, remove the lower two bits (to make it word-aligned)
     and simply add the offset between the heap and the shadow to find the state associated
     with `addr`.
  5. Note: This process is identical to your `memcheck` checker.  The only thing that should
     change are the variable names.

For `eraser.h`:
  - `eraser_fn` and `eraser_trace_only_fn` mirror the same routines you
    built for memcheck: run a routine at user level with checking and
    return the result it produced.  The `trace_only` implementation does
    not use shadow memory and simply runs the given routine (this lets you
    check that your basic setup and trapping is correct).  These routines
    should allocate and setup the page tables as in memcheck.  (I know:
    this is a ridiculous way to do things --- we will change this in a
    bit, I didn't want to change too much at once.)

  - `eraser_mark_alloc`: mark an address range as allocated.  Note that
    this range will always be at least 4-byte aligned and the `nbytes`
    always a multiple of 4-bytes.

    Note: by the time this gets called, `kmalloc` will have already
    `memset` the region to 0.  You may either want to initialize the
    memory region at the beginning to a new state (e.g., `IGNORE`) or
    just accept the somewhat confusing re-VIRGIN state transition that
    will happen.  I did the latter; but the former is cleaner.

  - `eraser_mark_free` will mark the region as `FREE`.  We don't actually need this
     for the current test because of how our `kmalloc` works.  In a full-functioned
     allocator, the `free()` implementation would likely be modifying the freed
     memory (to put it on a freelist etc) --- if Eraser does not know the block is
     freed it will give a bunch of false error reports.  
 
     One way to look at this is that once a pointer is passed to `free()`
     that block is now private (sort of like the Exclusive state in
     eraser) and so doesn't need locks to control access.

  - `eraser_lock` and `eraser_unlock` will be called by the thread package's
    locking routines.  This should add and remove locks from the lockset.

  - `eraser_mark_lock`: I was planning on using this for hea-allocated
    locks  to tell the tool to *not* track a lockset on a lock (since
    that makes not sense --- locks by design are read and written
    without holding a lock!).  Currently we just use global locks,
    so this doesn't matter.  This would have to be called by any lock
    initialization the thread's package does.  It may be better to just
    write `eraser_lock` and `eraser_unlock` in such a way that you don't
    have to do use this routine, however.

  - `eraser_set_thread_id` would be called by the thread's package to tell
    your tool it switched threads.

The tests are in `tests/part2-tests*.c`:
  - `part2-test0.c` (no error) --- basic non-eraser test that makes sure
    you can still run a routine at user level after refactoring.
  - `part2-test1.c` (no error) --- basic non-eraser test that makes sure
    you can still allocate, write and read heap memory.  
  - `part2-test2.c` (no error) --- simple eraser test that has one
    "thread" that acquires a lock, allocates memory, writes then reads
    memory, and releases the lock. Should have no error because the 
    memory is always protected by a lock.
  - `part2-test3.c` (error) --- similar, but does the read without
    the lock.   This will have an error because the we aren't tracking
    if state is shared.  
  - `part2-test4.c` (error) --- similar, but does a write without the lock.
    This will have an error because the we aren't tracking
    if state is shared.  

--------------------------------------------------------------------------------------
#### Part 3: Shared-exclusive Eraser

The previous Eraser is pretty useless.  Let us at least track if a second
thread is touching memory before giving an error!     We'll use the 
shared exclusive state as described in the paper 
for this.  

When a thread T1 touches word `w` in a the `SH_VIRGIN` state:
   1. Transition to `SH_EXCLUSIVE`.  (I misspelled as `SH_EXLUSIVE` initially --- oy.)
   2. Store the current thread id in the associated state.
   3. For subsequent accesses by the same thread T1, stay in `SH_EXCLUSIVE`.
   4. For subsequent accesses by a different thread T2, transition to the 
      `SH_SHARED_MOD` state and initialize the variable's lockset to T2's lockset.
   5. If the lockset in `SH_SHARED_MOD` becomes empty (even on the initial 
      transition), give an error.

The tests are in `tests/part3-tests*.c` --- the first two tests are the
same as above; if I was more clever we'd have a way to flip their behaivor:
  - `part3-test3.c` (no error) --- identical test as `part2-test3.c` but here we
    will *not* have an error because a second thread does not touch it.
  - `part2-test4.c` (no error) --- identical test as `part2-test4.c` but here
    we will not have an error because a second thread does not touch it.

  - `part2-test5.c` (error) --- similar, but the unlocked read is done
    by a second thread so you should flag an error.
  - `part2-test6.c` (no error) --- similar, but has the memory allocation 
    outside of the lock and only has one thread, so no error.

  - There are a bunch of other tests now, too --- just look at them :)

Note that the state structure only has a small 16-bit wod to track the
lockset (or in our case the single held lock).   I did this so that the
state could be 32-bits and the shadow memory calculations would be easy.
This does create a problem of how to map from a lock address to a small
integer.  I used the following dumb way to get from lock pointers to a
small integer that can be stored in a state:

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

As always, you are adults, so are welcome to do something less basic.

--------------------------------------------------------------------------------------
#### Part 4: Shared Eraser

Now add the shared state.  Recall this state was used to handle the common
case where one thread intialized data, and then subsequent accesses by
all other threads were read-only and thus did not use locks.  Tests are in
`part4-tests*.c`.  The basic idea: 
  1. On write in Exclusive you'll transition to Shared Modified (as above).
  2. On read in Exclusive you'll now transition to Shared.  In shared you keep
     refining the lockset, but do not give an error if it becomes empty.  If 
     a write happens, you transition to Shared Modified and (as always) immediately
     give an error for an empty lockset.

--------------------------------------------------------------------------------------
#### Extension: run at kernel level.

Mr. Han had the very reasonable point that our tool was kinda lame
because it only checked user-level code.  I figured out a way to run
most of the code at `SUPER` (kernel) level.

The following is from the lab `lab-real-world-tracing`:

   - We can get around the limit that ARM only allows single-stepping
     `USER` code by exploiting the following fact.

     AFAIK, the ARM memory operations behave the same no matter the
     privilege level.  Thus, running memory operations at user level
     and the rest of the checked code at its expected `SUPER` privilege
     should give the same results as running all of the code at `SUPER`.

   - We can exploit this fact as follows: When we get a domain fault
     trap, enable client access to the `track_id` domain (same as we
     do now), *however*, after setting up a mismatch fault (same as we
     do now) and before jumping back to the faulting memory operation,
     change the exception state (the saved stack pointer and the saved
     co-processor register `spsr` of the interrupted code) so that when we
     resume we will run that one single memory operation at `USER` mode.
     When we get the single-step mismatch fault (same as now), we change
     the exception state so resumption will take use back to `SUPER` mode.

   - In this way, we run all non-faulting memory operations at `SUPER`
     mode (where they expect to run and hence will give the same results),
     and only those memory operations that fault at `USER` mode (where
     such memory operations will behave the same in any case).  As a
     result, we can monitor all loads and stores without changing the
     behavior of the checked code.


The way you'll have to do this:
  1. In the domain exception, grab the `spsr` and change the mode to be `USER`.
  2. Get the stack pointer in `SUPER` mode (you'll have to switch to `SUPER` 
     grab the `sp` and switch back out).
  3. Set the `USER` stack pointer to the one you retrieved from `SUPER`.
  4. Return from the domain exeption.

  5. In the single-step exception do the reverse: swap the `spsr` to be `SUPER`
     grab the stack pointer from `USER` and set it in `SUPER`.
  6. Jump back.
     
This is a fun hack.

--------------------------------------------------------------------------------------
#### Find other race bugs.

Useful checks:
  - Track if locks are double acquired, releases without acquisition, held "too long",
     acquired and released with no shared state touched (this often will point out bugs 
     in the tool).
  - Deadlock detection by adding edges to a hash table and detecting cycles.

Both of these find real bugs in real code.  Recommended.

--------------------------------------------------------------------------------------
#### Other Extensions:  Tons.

There are obviously all sorts of extensions.  
  0. Handle multiple locks!  Multiple threads!  Multiple allocations!

  1. Handle fork/join code that has "happens-before" constraints (e.g.,
     code cannot run before it is forked, code that does a join cannot
     run before the thread it waits on has exited).  Adding these features
     can remove a bunch of false positives.

  2. The shared-excluse state is a bad hack  --- a thread can run for
     milliseconds (millions of instructions) before a thread switch.
     This potentially opens up all sorts of false negatives.  One easy
     hack to counter: use your single-stepping code to force a context
     switch after a small number of instructions (or, perhaps, as soon as
     the initializing thread returns from the initialization routine).
     A second is to use your garbage collection pointer scanner to
     determine which addresses can be seen by another thread (e.g.,
     as soon as they are reachable from a heap data structure).

  3. Add checking to detect when interrupt handlers read/write memory
     that threads are modifying as well.  I don't believe the eraser
     description works (but perhaps I am incorrec!) so you may have to
     think of an alternative approach.

  4. Lots of other ones to make this tool more useful.  It's obviously pretty crude.
