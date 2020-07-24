## Processes

Last lab built a single, trivial `init` process.  This lab will:
  1. Build a more full-fledged version so we can have more than one
     process, give it ownership of resources, share ownership, and handle
     exceptions in your library OS.

  2. Re-purpose your single-step code to test each pix modification we
     do so we can detect and eliminate many tricky bugs without trying
     hard.

##### VM Readings

I would suggest rereading cs140e's virtual memory lab 
[documents](https://github.com/dddrrreee/cs140e-20win/tree/master/labs/14-vm/docs).
Recall that:
  0. These are annotated, so you should at the very least skim through and read the
     annotations.  I will take a stab at adding some more.
  1. We are only using 1MB pages ("segments"), so you can skip the
     smaller page discussion (however, I think we will have you guys implement those
     so you can throw away our staff code).
  2. The document names prefixed with "armv6" are the general ARMV6 docs
     those prefixed with "arm1176" are for the specific chip we are
     dealing with.
  3. If you read every page, it's a lot, but the actual residue that
     you absolutely need is fairly small, so this won't be a crazy amount.
     I know you guys don't do this, but I would *strongly* suggest
     printing these so you can mark them up and easily flip through them
     in lab.   It's crippling to interface to them screen-only.

Suggested focus:
  - Read about how to have two page tables, one for system and one for
    user.
  - Understand the different kinds of flags we can set for page table
    entries both for protection and for speed.  You can look in the
    `armv6-` headers in in `libpi/include` for the flags we have.
  - Refresh your memory on the coherency we have to do when switching
    address spaces and inserting/deleting pages.  I gave you routines to
    call (in `our-mmu-asm.o`) but I think it makes sense in our upcoming
    speed lab to optimize these so you have your own version, but you
    aren't replicating what was done in cs140e.  (The provided methods
    always use the simplest, obviously over-kill approach to minimize
    bugs, but the speed hit is intense).

##### Background  Readings

I've added some background [readings](./readings/README.md) that discuss:
  1. Exokernels.   Suggested: the exokernel talk.  It's pretty quick.
  2. How to run untrusted code safely, which can be used to download
     application code into the kernel and run it.  Suggested: Native
     client.  Pretty interesting approach.

There's a bunch of different research papers; you don't have to digest
them all, but they are provided for context.  I will attempt to include
enough background in the different lab README's to get through them,
but it can't hurt to see a more professional take.

---------------------------------------------------------------------
#### Part 1: re-purpose single stepping.

***Still rewriting below and figuring things out.***

For today's lab, and in general, many of our pix modifications should
have no observable change to user process behavior.   Thus, an easy way 
to detect if a pix modification has a bug is to:
  1. Run a bunch of user processes before doing the modification, recording
     the instructions they run and their register values.
  2. Do the modification.
  3. Re-run the user programs, checking that the traces remain the same.

I think all operating systems should do this, but AFAIK no one has ever
thought to do this trick.  (At best they may compare `printf` output's,
which can miss many errors and is not informative about when exactly
the problem occured.)

We are going to set up your single-stepping code so that you can trace
each user-process, compute a running hash of what it is doing,
and then print the hash when it exits.  We use a hash rather than logging
all operations because:
  1. The hash will be sufficient to detect errors.
  2. It sidesteps the need to have a logging framework and allocate memory for the 
     moment (since pix is in flux).
The downside will be that a hash isn't super informative for debugging
since you won't know when the divergence happened.  We will remove this
limiation when we decide on how to do kernel memory allocation.

As you make modifications, different levels will stay the same --- when
you modify the libos, you can't compare directly.  However, you can
compare the system call traces across implementations.  We will modify
the tracing code to only log system calls so that you can do this easily.
can compare across different implementations.

We may wind up moving the tracing code to the libOS itself, since that
is more exokernel-ish.  But for the moment we stick it in pix.
