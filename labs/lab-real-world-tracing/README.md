## Making tracing more real-world

For this lab, we'll make your tracing much more lightweight and
real-world.  Hopefully by the end you can easily use your memcheck tool
whenever you hit a bug in your code.

Currently our memtrace tool (and thus everything built on top of it)
has some big limitations:

  1. Forces code to run at user-level: currently we can only check
     kernel code by running it at user level (so we can use
     single-stepping).  On the ARM, you often can't simply run privileged
     kernel code as unprivileged user code and get identical results:
     Some instructions silently behave differently when run with and
     without privileges.  Such silent difference in behavior is the
     same reason that ARM is not classically virtualizable (using a
     trap-and-emulate approach).

  2. Client modifications: currently you have to manually modify the
     checked code to call whatever tool you want to run, which prevents
     easy after-the-fact monitoring.

  3. Changes client addresses: Because we include the tool, the addresses
     in the checked program will often differ significantly from the
     unchecked program.  This change is bad for many reasons, one of
     which is that it can easily hide bugs.  For example, if you are
     running your OS code and a wild write corrupts a specific address
     `addr` when you link in your memcheck tool to find it, the code
     will change enough that the write may go somewhere else, enough so
     that you don't see it at all.

   4. Code can still corrupt many locations: because of how the code is
      laid out, we don't even catch null pointer reads and writes
      (page 0 is not protected), nor writes to code (code segment is not
      protected), nor writes to read-only data.  We will have to change
      the address space layout some to handle these issues.

   5. The way that the tool sets up address spaces is ridiculous.
      We make this less so.


Our solutions:

  1. How to check `SUPER` level code.  We can get around the limit that
     ARM only allows single-stepping `USER` code by exploiting the
     following fact.

     AFAIK, the ARM memory operations behave the same no matter the
     privilege level.  Thus, running memory operations at user level
     and the rest of the checked code at its expected `SUPER` privilege
     should give the same results as running all of the code at `SUPER`.

     We can exploit this fact as follows: When we get a domain fault
     trap, enable client access to the `track_id` domain (same as we
     do now), *however*, after setting up a mismatch fault (same as we
     do now) and before jumping back to the faulting memory operation,
     change the exception state (the saved stack pointer and the saved
     co-processor register `spsr` of the interrupted code) so that when we
     resume we will run that one single memory operation at `USER` mode.
     When we get the single-step mismatch fault (same as now), we change
     the exception state so resumption will take use back to `SUPER` mode.

     In this way, we run all non-faulting memory operations at `SUPER`
     mode (where they expect to run and hence will give the same results),
     and only those memory operations that fault at `USER` mode (where
     such memory operations will behave the same in any case).  As a
     result, we can monitor all loads and stores without changing the
     behavior of the checked code.

  2. How to transparently run the tool without modifications.  We want
     to simply run `memcheck hello.bin` the same way you would do
     `valgrind hello`.  The way we take the tool binary and the checked
     binary and merge them into one binary that the existing bootloader
     can handle, but in such a way that the checking tool gets control
     first.  The checking tool will then copy itself out of the way in
     the address space, copy the checked code where it expects to run,
     set up page-tables and any other state needed, and then run the
     checked code with full tracing.

     Doing this will require writing a new linker script that manages
     the two different binaries and automatically calculates information
     we need, as well as locating the code that does the copying using
     code linked at the expected `0x8000` location.  This will involve
     reading up on linker scripts, which is useful.  I will try to have a
     bunch of checksums computed to allow sanity checking (still trying
     to think of the best way to do this given that many of us have
     slightly different ARM compilers --- annoying!).

   3. The hack above will also preserve client addresses.  If you had a 
      bug at a given address without checking, it should show up at the 
      same address with checking.

   4. Stopping null pointer reads and writes and code writes:  This
      will involve either changing the client linker script to use a
      different layout (so we can use 1MB pages) or using small pages.
      Still debating.  Small pages let you guys write your own VM
      implementation, which is useful.

   5. Setting up addresses more sensibly.  This should be a matter of
      hacking `cstart` --- TBD.

Readings:
   1. The `valgrind.pdf` paper has a good short discussion of how they laid things out
      in the checked code's address space.
   2. The ATOM paper does as well.

The issues these tools had are the same we have with slightly different
constraints (since we run in the kernel).
