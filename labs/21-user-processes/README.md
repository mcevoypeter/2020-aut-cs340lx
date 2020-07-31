## Processes

Last lab built a single, trivial `init` process.  This lab will: 

   0. Increase your r/pi memory size so that it's worth managing.
   1. Build a more full-fledged version so we can have more than one process, give
      it ownership of resources, share ownership, and handle exceptions in
      your library OS.

--------------------------------------------------------------------------
#### Background: page table stuff (read this)

It may help clarify various magic numbers later to keep the following
derivation in mind:

  - The ARM CPU allows any 32-bit register to be used as a pointer.
  - Memory is byte-addressable.
  - Thus, we can enumerate 4GB bytes.  
  - Since we use 1MB pages ("sections" in ARM parlance)
    and a single-level flat page table (defined by the hardware) we know
    we need 4GB/1MB=4096 entries to cover the entire address space.
  - Since each page table entry is 4-bytes, this means a full page table
    (one that can cover the entire address space) is 4k*4 = 16k bytes.
  - We are going to use a small number of processes, so you can statically
    allocate enough page tables on the single page we currently use (at
    `0x10000`) for all of them, which is handy.

Each page table entry has a bunch of useful flags.  In addition to the
readings listed below, the following header files are useful:

  - The PTE structure for our page table (which uses 1Mb sections) is in
    `libpi/include/armv6-vm.h`.  

  - `armv6-cp15.h` has prototypes for numerous useful cp15 routines.
    These are provided in `our-mmu-asm.o` --- for this lab just use
    ours, but you should have an eye on writing your own  at some point.
    The header `armv6-coprocessor-asm.h` has many of these instructions
    already setup.

--------------------------------------------------------------------------
#### Background: TLB stuff (read this)

Given that address translations can happen on every instruction fetch
or even more often (for load and stores) translation can easily be a
bottleneck.  To try to control this, modern machines use a cache called
(confusingly) a "translation lookaside buffer" (TLB) to cache page table
entries so they don't have to look these up in memory.

Since processes usually have private address spaces, they frequently map
the same virtual page to a different physical page.  For example, if you
`fork` our `init` process it will have a translations for virtual address
`0x800000` that maps it to a different physical page.  Caching such
translations would cause incorrect results if the translation for one
process P1 was cached and we switched to P2 and it accessed the same
address --- the TLB would incorrectly use P1's translation.  

The simplest solution: flush the TLB on every context switch.  Easy,
but potentially costly in terms of subsequent page table faults if we
switch often.  As a result, most modern architectures will tag each
TLB entry with a unique small integer, an "address space identifier"
(ASID) that corresponds to the owning process.  (A common number of
ASIDs is 64.)  When you switch address spaces, instead of flushing the
TLB, you simply switch the current ASID, and the hardware will only use
entries with that ASID tag.  You only need to flush entries for an ASID
when you reuse it, such as when a process dies and you realloce that
ASID or are more processes than ASIDs, so you have to cycle between them.

ARM ASID factoids:
  - ***NEVER USE ASID 0***: as you may recall from our virtual memory
    caching lab, we follow the ARM suggestion and use ASID 0 to handle
    some cache coherence issues.
  - The ARM has 64 ASIDs.  
  - You will have to track which process has which ASID and correctly switch this 
    when you switch address spaces.  
  - You'll also (as stated above) have to flush when you could reuse it.

  - The ARM TLB, like all(?) TLBS lets you pin TLB entries so they cannot
    be replaced.  An interesting embedded hack (that I don't know of
    people using): for small processes, you can get rid of page tables
    entirely and simply pin the few TLB entries the process needs so that
    it never gets a TLB fault (and hence does not need a page table).
    We might do this for fun later.  Makes processes very lightweight and
    small.  We could perhaps use this hack to make thousands of processes.

  - As a clarification: since we currently do not pin TLB entries for
    correctness we could simply ignore the ASID and always flush the
    TLB on every switch.  Things should still work. Similarly, we could
    always switch to a virgin unused ASID on every context switch (or,
    even, on every instruction).  Switching ASIDs can be used as a
    quick cache cleaning method in some cases.  (Exmaple uses: to test
    your cache cleaning / invalidation code or for getting performance
    counts on repeated runs of a routine). 

--------------------------------------------------------------------------
#### Background: VM Readings

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
#### Part 0: figure out how much memory we have

The box our r/pi came in claims 512MB of physical memory (as does the r/pi
foundation website).  *However*, depending on how parameters are set, a
good chunk of this memory can be claimed by the GPU and thus not available
for our code running on the CPU.    So before we start messing with 
physical memory more, let's actually verify how much we have.

There are two ways I know of:
   1. [ATAGS](http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#ATAG_MEM)
      which is a primitive take on a "key-value" store used by the pi
      firmware bootloader (`bootcode.bin`): it writes these out starting
      at address `0x100` (allegedly it will also pass this address in
      register `r2` but I have not checked).  You can traverse these
      until you hit `ATAG_NONE`.  When I did so, the pi claimed it had
      128MB of physical memory.  This was way too small, so I thought
      I had misinterpreted (or the bootloader had screwed up).

   2. [mailboxes](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)
      gives a way to send messages to the GPU and receive a response.
      You can use this to query and configure the hardware.  The mailbox
      interface is not super-intuitive, and the main writeup uses
      passive-declarative voice style that makes it hard to figure out
      what to do.  (One key fact: the memory used to send the request
      is re-used for replies.)  When I got the mailbox working, it also
      claimed 128MB.

So, two different calculations, same astonishingly tiny result ---
starting to think it was computed right.  This is bad, since we some
of our interrupt stack addresses are above 128MB!   It's unclear
what the consequence of using memory the GPU thinks is for it, but a
likely result is that the GPU can randomly corrupt our interrupt stack.
(Since we weren't using graphics, perhaps we were getting away with this,
but it's wildly bad form.)

So we're do things right:
   1. Write code to get what the GPU thinks your physical memory size is
   ("Trust but verify --- Stalin.)

   2. Do what you need to to increase it. 

##### Writing mailbox code to check memory size (and other things)

If you look through the (unfortunately incomplete) 
[mailbox writeup](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)
you'll
see all sorts of useful facts you can query for --- model number, serial
number, ethernet address, etc.  So it's worth figuring out how to do it.

So that's what we will do.  Some hints:

   1. As always: if you want to write the code completely from scratch,
      I think doing so is a valuable exercise.  However, in the interests
      of time I put some starter code in `part0-mailbox/`.  Extend it
      to query for physical memory size along with a couple of other
      things that seem useful.

   2. For the life of me, other than that wiki page, I cannot find any 
      information in the main Broadcom PDF.  [This blog](http://magicsmoke.co.za/?p=284) does have a clear writeup, but it would be nice to have a primary source:

   3. I included a simple `ATAG` implementation --- this isn't a super
      useful interface on the pi since it (appears to) only provide three
      different key-value pairs.  However, the approach is a neat (albeit
      gross) hack for passing arbitrary key-values in an extensible way.
      Funny enough, I had "re-invented" something similar as a way to pass
      information from our bootloader to the process that it started.
      You might want to keep this hack in mind for your later systems.
      Gross, but pretty robust.

Rules:
  1. Buffer must be 16-byte aligned (because the low bits are ignored).
  2. The response will overwrite the request, so the size of the buffer
     needs to be the maximum of these (but not the summation).
  3. The document states that additional tags could be returned --- it may
     be worth experimenting with sending a larger buffer and checking
     the result.

##### Increasing memory by replacing the firmware

Ok, our memory size sucks: So how to change it?   I spent a surprisingly
long time fighting with this problem going in circles on forum/blog posts:

  - Generally people claim if you modify `gpu_mem` in `config.txt` (the file on 
    your SD card) this will change how much memory the GPU uses.  Unfortunately,
    doing *only* this change had no effect, at least for our firmware.

  - I then found a forum post that stated you can use a different
    `start.elf` files on the SD card to do the partitioning.  (This was
    a WTF of supreme magnitude: such a bad design to require replacing a
    random black-box file in addition to modifying `config.txt` especially
    when the official documents seem to not mention the need to do so!)
    However, it was unclear where to get this magic file and there were
    tons of later posts stating that this was an old method so don't
    do it.  Unfortunately, I tried the "new" approach with no success.

  - Finally, I went back to trying to use different `start.elf` files ---
    it turns out that this replacement *is* a valid method, and at
    least for our bare-metal approach, possibly the *only* valid
    method.  Credit to this post on making [the pi boot as fast as
    possible](https://www.raspberrypi.org/forums/viewtopic.php?t=84734).
    Without it, I might have given up.

So, great: let's change your memory to a reasonable amount from "first principles":

  1. The [elinux](https://elinux.org/RPi_Software) website describes the different
     `start.elf` file options.   They state there is a stripped down one that works
      with `gpu_mem=16GB`.   

  2. Go to the [linux firmware site](https://github.com/raspberrypi/firmware/tree/master/boot)
     and download the right `start` and `fixup` file.  

  3. Copy these files to your SD card, and create a new `config.txt`
     that is simply:


            # config.txt: save the old one!
            gpu_mem=16
            start_file=start_cd.elf
            fixup_file=fixup_cd.dat

  4. Plug the SD card back in your pi (after doing a `sync`) and re-run
     your mailbox code to see that the memory size: it should be 496MB.

Great!   You may well have saved a couple of annoying hours and now have
almost 4x more physical memory.  With that said: there are potential
downsides to this increase:

  1. If you ever do graphics and need the GPU, you will need to increase memory 
     back up to as usuable number --- 16MB is far too small.
  2. More memory bookkeeping data structures, including the work mapping
     physical memory into each process's address space (at least until
     we use a single system page table).  If you were making a shippable
     system, counter-intuitively you might want to artificially limit
     the memory available so that it could be more stripped down / faster.

---------------------------------------------------------------------
#### Part 1: re-factor your code

***UNFORTUNATELY you will have to run your regressions by hand until
I can hack the makefile.***

Before making `fork`, there are some simple refectoring steps you should
make to your `init` code.  We describe these below.  ***Before you modify
anything***, go into today's `code/` and run:

        make emit-all
        make check-all

This will emit output for the test code and then compare it.  After any
restructuring step that you have a working system, then run:

        make checkall

and verify it completes.

Make the following changes and run the checks after each one:

###### 1. Do correct global / not-global mappings.

The page table has an `nG` field that specifies if a mapping is global
(0) or private (1).  A TLB entry with `nG=0` (global) will match a
translation request in *any* process address space.    In our setup,
we want to alias your OS pix into every process address space so that
when a system call occurs we can immediately start referencing memory
without doing an additional address space switch.  Thus, we will mark
all OS memory as having a global mapping.  Conversely, we want user
memory to be private so want to mark it as such.

NOTE: You should *never* have the same virtual address range marked
not-global in one process page table and global in another --- this
will lead incorrect / undefined behavior depending on how the entries
are fetched.  (Verifying this invariant is a useful check to run 
across your page tables.)

What to do, when you insert entries in your page table:
  1. Mark all process-specific mappings 
     private (`nG=1`).  Given that we are running a 1MB `init` procesas
     and using 1MB sections, there should be exactly one such mapping.
  2. Mark all OS mappings as global (`nG=0`).

How I did it: I changed `mmu.c:mmu_map_section`  to make it private by
default and added a new routine `kern_map_section` that would make the
mapping public.

After this change your checks should work as before.

###### 2. Allocate / track physical sections.

We currently just reference and use random physical memory locations.
This is a recipe for disaster.  You should now switch your code to
explicitly allocate, reference count, free physical sections so you
have a legitimate bookkeeping of what is in use.  I checked in a simple
reference counting implementation in `libpi/libc/refs.h` that you can
use --- it's stupid but should be correct.

I added calls to this code to the mapping routines above. 

###### 3. Put everything in an environment.

We'll put everyting for a process in the `env_t` structure in `pix.h`.
I statically declared `MAX_ENV` of these (in `pix-internal.h`) and used
the convention that a null `init` meant it was free.

You'll need:
  - A address for `init` (this is the address you jumped to for init).
  - A unique pid (I just incremented a global counter that started at 1).
  - A unique ASID that is not 0 and not shared with any other env.    You
    may want to use the `refs` vector to track this.
  - A pointer to the page table.  Each page table is 16k bytes and
    we won't have more than 64 of them, so I placed all page tables
    on the 1MB page starting at `0x100000` (as now) back to back.
    That way their physical and virtual address is the same.  Make sure
    you allocate this page so it doesn't get used for anything else!
  - For today we won't use the registers.

You should set up your routine that would jump to the init code in the 
last lab to work if you call it with an environment.


###### 4. Fix the call to `set_procid_ttbr0`

The original `set_procid_ttbr0` call looks something like:

    set_procid_ttbr0(0x140e, dom_id, pt);

which is totally broken but just happened to work.   You should change
it to use the ASID from the current `env` rather than `dom_id` (as well
as it's pid instead of `0x140e`).  The original call happened to work
because `dom_id` was 1, which is a perfectly fine ASID.  

After you run this, you should be able to check that the values are what
you expect

        // use assertions to check.
        printk("asid=%d\n", cp15_procid_rd() % 64);
        printk("tlbr0=%p\n", cp15_ttbr0_rd());
        printk("ttbr1=%p\n", cp15_ttbr1_rd());


Note, that the `pid` value is uninterpreted by the hardware.  It just
stores and returns it, it will not use it.  (We won't either, but
it can be useful for sanity checking.)



---------------------------------------------------------------------
#### Part 1: add sys_clone (or sys_fork)

For `sys_clone` you'll need a couple of extra things:
  1. Alias all of physical memory to a known offset so that if you want to read
     or write any physical address you can just add the offset to the address
     you want to write.
  2. Add the `sys_clone` system call.  The main issue on the pix side is
     duplicating the page table.    The main issue on the libos side is
     writing a special trampoline that can save state before the syscall
     and resume after (since the kernel does not).
  3. Fix `sys_exit` to run another process if there is another one.

###### 1. Alias physical memory.

As you'll discover in the next step, you often want to write to memory
in a process that is not running.    In theory you could change address
spaces to do so, but if you want to write from process A to process B
this doesn't work well.

Instead many OSes use the following hack:
  1. Alias all of phsical memory (as one contiguous chunk) to a known
     offset as a global mapping that will be valid in all address spaces.
     For an offset we use `PHYS_OFFSET` which is `0x80000000` (defined in
     `pix-constants.h`)

  2. When kernel code wants to write to physical memory, it simply
     adds this
     offset to the address.

For exmaple, to copy one physical section to another when the MMU is on, simply
do:

    void *phys_addr(void *addr) {
        assert(addr < PHYS_MEM_SIZE);
        return (void*)((char*)addr+PHYS_OFFSET);
    }
    void copy_section(void *to,  void *from) {
        memcpy(phys_addr(to), phys_addr(from), OneMB);
    }


###### 2. Implement `sys_clone`

On the pix side:
   1. You'll have to duplicate the page table.  You'll scan through it and for
      every global entry, just copy that entry to the new page table.  For
      every non-global entry you'll have to allocate a new page and copy
      the physical contents (using a `copy_section` like the above).
   2. You'll have to set the `init` to be the location passed into `sys_clone`.

On the libos side:
   1. Pix does not save any state other than the continuation pc to jump back to.
      So you will have to save this before calling.
   2. You will also have to load the state after the call.
   3. The trick is that you won't know what the stack pointer was set to before the
      call so you will have to store the stack in a global variable.
   4. To compute the address of where to jump back to, I just used an `ldr` of a label
      (look in the interrupt code to see how to do this) since I know for sure what
      value will get loaded.  If you copy the pc over, it's value is 8 bytes ahead.

If the code does not work, I instead passed in the address of a routine that:
  1. Loaded a fresh stack pointer.
  2. Called  `sys_putc` so I could see that the code was running at all.



     duplicating the page table.    The main issue on the libos side is
     writing a special trampoline that can save state before the syscall
     and resume after (since the kernel does not).
  3. Fix `sys_exit` to run another process if there is another one.

###### 3. Finish `sys_exit`.

Right now `sys_exit` just reboots.  You should change it so that
it:
   1. Looks for the next runnable process (i.e., an `env` with a non-null `init`).
   2. If there is none, reboot.
   3. Otherwise, delete the current process including its private pages
      --- these should have a refcount of 1) and then call `schedule()`.
   4. Schedule just jumps to the new process.

Note that we all of our kernel calls are run to completion.  As a result,
we *never* save state.  At the next call, we load the stack pointer to
the start of the stack and run from scratch.  This makes entry and exit
from the kernel fast (no state to save and restore, no stacks to manage).
