***Note: this readme is still being written.  It's worth reading, but don't
get surprised if some stuff isn't that clear.***  

This is from the first lecture of 140e --- but after reading the "threads
can't be implemented as a library" paper I think you'll have  a much
more subtle understanding of the code.

### The "As-if" substitution principle.

Bishop Berkeley is the patron saint of computer stuff.  If a tree falls in 
the forest and no one is there to hear it,

   - then it could be stored in a register.
   - or could be skipped.
   - or rendering could be deferred until someone walks by.

A famous equivalance-substitution example that uses "as if" is the
Turing test.

As-if is a core systems principle.  And weirdly is not really talked about.

   - Over-simplifying: given a program P, if program P_better has the same
     observable behavior ("side-effects") as P does --- i.e., it behaves
     "as if" it was P ---  then we say they are equivalent and, for example,
     we can replace P with P_better.

### How as-if lets the compiler wtf-you.

When you write code, you likely reason about how it behaves by looking
at the source code (loops, variables, function calls, etc).   

  - Footnote: It's worth thinking about how you actually reason about what
    your code does.  This likely involves working forwads or backwards
    following a sequence of how/what each storage location (variable,
    heap) is assigned, what is read, and what side-effects occur.
    You are likely informally computing what "happens before" (in the
    Erase paper sense) to determine what causes what.  Your reasoning is
    likely sequential and tries to ignore stuff that is (hopefully)
    not relevant, such as the 100+ other programs executing on your
    computer when this one is.

However, obviously, hardware can't run C/C++/Rust source directly.
Thus the compiler translates your source code down to low-level operations
that will behave "as if" it was the original (sort of: only if your
program was well-defined).

As part of this translation, compilers aggressively optimize code by
replacing chunks of it with puportedly faster or smaller equivalants that
behave "as if" they were the original.  The compiler may even delete
the code (dead code removal) if it believes an external observer could
not tell.  A very small set of examples:

   - inlining.
   - common sub-expression elimination
   - redundant read elimination.
   - reorder reads and writes.
   - code hoisting.

The key issue here is what an "observer" is.   If the observer can
tell the difference between P and P', then they are not equivalant.
Of course, an observer with complete information (such as your CPU,
a debugger, or even a disassembler or `diff`!) knows exactly how code differs ---
if we had to satisfy the equivalency judgement of these observers, we
could never optimize anything, because they could detect any change.
The game people play is to --- perhaps counter-intuitively --- try to
make the observer as weak as possible, since it can be fooled the most
number of ways.  Of course, the kicker is "as possible": the resultant
code must still do what it was supposed to.  For example: perhaps it
must perform exactly the same externally visible side-effects in the
same order (network messages, disk writes, output printing).

    It's a shame I don't have the technical expertise to discuss this
    deeply, but there is a very strong, very unremarked-upon parallel
    between systems and quantum physics.   As systems designers
    and hackers we constantly play observer games.  Our observations
    constantly Heisenberg-perturb what we observe.  And we deliberately
    live in a universe thoroughly built upon many low-rent simulacra of
    the Heisenberg cat experiments, where we completely avoid making a
    cat, either living or dead, until you open the box (observe) it since
    before that, anything (or nothing) is the same as the right thing.

Roughly speaking, the C compiler `gcc` that we will use  assumes that
only the code you write can affect or observe values, and only do so
sequentially.  The memory model of the language (the set of values a
read of a variable is allowed to return) is sequentially consistent:
each load must return the value of the last write.

It's a bit tricky to say exactly what the observers are, but a reasonable
approximation is that *internally*:
  1. A program observes by reading variables (including dereferencing pointers,
     which is why alias analysis is so crucial).
  2. We can defer any calculation of that variable's value until the read occurs.
  3. We can also push the read later in the program.

With that said, the only real observations we care about are those done
by an external obsever, based on program side-effects --- what essages
it sends, disk writes or GPU operations it performs, device actions it
causes, etc.  Current compilers have a limited view of such things,
so typically do a simple appoximation where it makes sure that the
calls a *well-defined* program does to "opaque" functions (for which
it does not have source) will always occur identically (same order,
same values) after any translation it performs.  For similar reasons,
that reads and writes of `volatile` memory are similarly identical.

Stepping back.  If we have code that does:

    y = 1                   
    x = 2;

`gcc` can reorder these as:

    x = 2;
    y = 1                   

Or even delete both if it sees no reads of them.

Of course, if there is another thread running concurrently, this thread
most certainly can tell the difference.  

For this class, often you will communicate with hardware by reading and
writing values to specific, shared memory locations.  Typically, the
order of reads and writes to these locations matters very much.  Thus,
this hardware is both a strict observer and a mutator of these locations.
Unfortunately, `gcc` has no idea that there is an external agent that can
see or affect these values, and so its optimizations can totally destroy
the intended semantics.   (You cannot detect such problems without looking
at the machine code it generates; which is a good reason to get in the
habit of doing so!)

One method to handle this problem is to mark any shared memory location as
`volatile`.  The rough rule for `gcc` is that it will not remove, add,
or reorder loads and stores to volatile locations.

A bit more precisely, from the very useful [blog](https://blog.regehr.org/archives/28):

 - Use `volatile` only when you can provide a precise technical
 justification.  `volatile` is not a substitute for thought

 - If an aggregate type is `volatile`, the effect is the same as making all
 members `volatile`.

 - The C standard is unambiguous that `volatile` side effects must
 not move past sequence points

As a more cynical counter-point from someone who should know:

    "The keywords 'register' 'volatile' and 'const'are recognized
    syntactically but are semantically ignored.  'Volatile' seems to have
    no meaning, so it is hard to tell if ignoring it is a departure from
    the standard.

            Ken Thompson, "A New C Compiler"

It's easy to forget to add `volatile` in each place you need it.
Even worse, if you forget, often the code will almost-always work and
only occasionally break.  Tracking down the problem is a nightmare (add
a `printf`?   Problem goes away.  Remove some code?  Same.  Add some
code?  Same.)  As a result, for this class we only ever read/write
device memory using trivial assembly functions `get32` and `put32`
(disussed in lab `1-gpio`) since this defeats any attempt of `gcc`
to opimize the operations they perform.

However, looking at code generated by the compiler that is affected by
`volatile` issues is a wonderfully illuminating, concrete way to see
the gap between the fake abstraction of a programming language and
what the machine actually runs.  (One of the goals of this class is to
frequently have you rip back the lies of abstractions and see something
closer to reality.)

### Examples

For today we will:
  - Go through the examples in the `volatile/` directory.

  - Get a feel for compiler observation games, by going through the 
    `pointer` directory examples.  Each dereference of a pointer is a
      observation --- the compiler can only optimize if it's sure no pointer
      dereference can catch it.
  - To refresh your view of C, look through the `c-traps` directory.
