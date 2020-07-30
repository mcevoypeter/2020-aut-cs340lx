Since I mentioned exokernels, here's a few papers on it:

  - [job talk](exokernel-job-talk.pdf): this is a pretty fast, lightweight view.

  - [initial exokernel](exokernel-sosp95.pdf): this is a
    get-hit-by-a-truck initial paper.  It's dense, but lays out the
    philosophy.

  - [final exokernel](exokernel-sosp97.pdf): this is the final exokernel
    paper we wrote.  It has legit application performance (mostly due
    to the great Greg Ganger who built the webserver and the CFFS file
    system) and some more experience.

    Weird factoid: The use of downloaded code for UDFs is probably the
    single best technical trick I've come up with, but in fact didn't
    lead very far (yet?) :)

You don't need to read these in detail, or even at all --- I'll try
to make things self contained --- but they should give enough detail
to figure stuff out.  I'm hoping we build a similar virtual memory and
exception system for our OS.

Alex asked about making GPIO faster.  One way is to download application
code into the kernel and run it there.  Obviously you have to make
it safe.  These papers have examples of different, related ways to do it.

   - [sfi](./sfi.pdf): The original software-fault isolation paper.   It's been
     superceded by other work, but it's easy to understand and led to
     a lot of different efforts.

   - [native client](./cs343-annot-nacl.pdf):  a very thorough, impressive
     effort that did a form of SFI on the x86.  The project has since
     been cancelled by Google, but was great technically.  I believe
     it is superior to webasm in almost everyway (the one exception is
     that it is tied to x86, but you could imagine shipping fat binaries
     since ARM + x86 is basically everything).

   - [ashes](./ash-sigcomm.pdf): this is one way we used downloaded code
     in the exokernel to make fast message handlers.  We went with a
     fake assembly language (so it was easy to verify) and then JITed
     it to machine code (using methods similar to lab 2 in cs240).

   - [dpf](./dpf-sigcomm96.pdf): this uses dynamic code generation to
     make packet filters fast.  If I've understood correctly you can
     see similar techniques being used in the eBPF packet filter that
     has been adapted for performance monitoring in the Linux kernel.
     The useful thing from the paper is how crazy you can get with
     optimizations when you have a very narrow language.
