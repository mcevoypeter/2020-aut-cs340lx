# List of bugs
- Popping fewer registers than were pushed.
    foo:
        push {r0-r12, lr}
        @ code
        pop {r1-r12, lr}
        bx lr
- Using r2 to stash a value before issuing a swi instruction
    foo:
        @ code 
        blx r0
        mov r2, r0
        @ set-up code for swi 
        swi 1
        mov r0, r2
        pop {r2-r12, lr}
        bx lr
