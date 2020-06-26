0-pointer.c
    - goes away, no observer.

1-pointer.c
    - can't go away.
    - bar could observe since we don't have source and the definition
      can escape.


2-pointer.c
    if you make static?

    now it goes away exept for the call

3-pointer.c
    can you remove either write?

    i don't think so --- can be stored in the heap or in something else.

    "opaque" fucntion as in boehm.

4-pointer.c
    can it remove all this stuff now?

    yes.   it sees nothing is going on.  (this is an example of how going opaque
    to not can matter -- i can see in the box)

    bx lr!!

5-pointer.c
    wow, it emits one at a time

6-pointer.c

    if p == q ---> only need last write.
    if p != q ---> need writes to p and q, 

    can skip the first q write tho: why?
        how would you change it so you needed it?

        i think a call to an opaque. [yup]

7-pointer.c
8-pointer.c
9-pointer.c
10-pointer.c
