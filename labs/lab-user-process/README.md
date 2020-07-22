## Simple user processes

Perhaps surprisingly, you've already built up most of the pieces you
need for user processes --- system calls, running code at `USER` level,
virtual memory.  This lab packages them into a more coherent, explicit
user-process interface.

We're going to build our code so that you can compile a pi program
*without modification* to either:
  1. Run identically to how our `.bin`s work now --- as the sole program
     running on the hardware in `SUPER` mode, with complete control over
     the hardware.

  2. Run at user-level and call system calls do to actions it cannot
     do directly, such as privileged instructions, co-processor
     modifications, page table or TLB changes, etc.

After the lab you should be able to take old cs140e or cs240e device
labs and recompile and run them at user level.   (Note: we will have
to add tricks to for interrupts and hard real-time guarantees needed
by neopixels.)


NOTE: 
   * As a bonus, after doing this lab, we may (I think) be a good part of
     the way to being able to make a simple virtual machine monitor (VMM)
     implementation and then run your old `.bin` programs without any
     modification or even recompilation.   We will have to handle some
     additional issues, but I'm hoping we can do a bunch of programs in
     three ways --- either as kernel programs (as now), as user programs
     (as above) or as guest OSes running on top of a VMM.  Doing each
     three ways, without modifying them, gives a really good understanding
     of the different approaches and their tradeoffs.


We'll do something sort-of based on exokernel stuff in that much of the
OS functionality will be in an unprivileged library (a "library OS")
and the kernel will mostly do protection stuff.  The goal here is to
take as much resource management as possible out of the kernel (where
it can't be modified by unprivileged code) and put it into a library
where anyone can tweak, replace or swap out different implementations.
The kernel still provides protection so that one application cannot
destroy another.

I haven't thought about exokernels in about two decades, so no guarantee
I'll be talking about The Approved Way to do things, but it's an
interesting exercise for embedded --- compared to applications on a
general purpose OS, embedded apps often want more control, have to deal
with more constrained resource limits, higher cost of bugs (no one even
likes it if their toaster crashes, much less their card) and strict
time requirements (latency, guarantees) that simply don't exist in the
general purpose setting.


-----------------------------------------------------------------------
### The basic idea


You'll have to do several things:
  1. Implement a system call for each `libpi` operation you do that
     modifies either privileged hardware or kernel state.  In many cases
     you will not make an identical system call as the pi but instead
     decompose the implementation into smaller pieces that become low
     level system calls.  Doing so will often give more flexibility.

  2. Write a new library OS that implements `libpi` functionality at
     unprivileged level either by calling system calls outright or by
     building up equivalent functionality from smaller ones.
  3. Setting up kernel state well enough that you can run the first program.

We will build several interesting pieces:

  1.  A `fork` implementation that forks (duplicates) the current
      running process.  Doing so is mentally a bit fancy because unlike
      all other `fork()` implementations, this one clones the address
      space that it itself is running in.  It's not much code, but you
      have to think.

   2. A simple `pipe` implementation that can send and receive data from
      a different address space.  From this you can build arbitrary
      communication.   This is not hard to build but its an interesting
      piece to have --- in particular, I'm hoping we can tune it so that
      our performance ping-ponging data between processes on the pi is
      significantly faster than your MacOS or Linux implementation on
      your laptop can do.

   3. Signal handling so that you can receive different exceptions at
      user space quickly.  Similarly to fork, I'm hoping that --- as in
      the original exokernel paper --- we can be significantly faster
      than a general purpose OS.  If not immediately, at least after
      some tuning.

I think we'll build this in two steps:
  1. Make a user-libpi that simply forwards most things to the kernel libpi.  We will
     use this to test that the basic low level pieces work.
  2. Then add concepts of address spaces, processes, and such things.
     After building this libOS-libpi we should get the same output
     as user-libpi.

In much of this we will be using memory tracing to make sure that
all GPIO output (or all output at all) is the same as we do different
modifications.
