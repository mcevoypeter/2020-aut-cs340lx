## Eraser

Today we're going to build a simple version of eraser.  To keep it simple,
we will use fake threading calls that indicate:
  - What thread id is running.
  - When a context switch occurs.
  - When a lock and unlock occurs.
  - When you allocate and free memory.

We do fake threading to make it easier to develop your tool in just a
few hours.  Among other things, doing it fake rather than real makes
it easy to seperate out issues where you get race condition in your
tool, makes deterministic debugging trivial, and also side-steps issues
arising from bugs that you might have in your context switching or thread
package library.

Don't fear:  Once the tool works on fake code we will then do pre-emptive
threads and retarget it.  Tentatively this is planned for next Tuesday
in order for us to first build a exact cross-checking tool (on Thursday)
that can record the read and write sets of different pieces of code and
their instruction traces, which can then be used to compare different
runs for equivalance.  This tool will make it easy to ruthlessly
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

--------------------------------------------------------------------------------------
#### Part 2: Simple Eraser

Here, we just track the locks held during any modification and give an error if the 
lockset goes to empty.  
   - The tests are in `tests/part2-tests*.c`
   - For simplicity, just assume that we have at most 32-locks in the program.

--------------------------------------------------------------------------------------
#### Part 3: Shared-exclusive Eraser

Now add the shared exclusive state.  Tests are in `part3-tests*.c`.

--------------------------------------------------------------------------------------
#### Part 4: Shared Eraser

Now add the shared state.  Tests are in `part4-tests*.c`.

--------------------------------------------------------------------------------------
#### Extensions:

There are obviously all sorts of extensions.  
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
