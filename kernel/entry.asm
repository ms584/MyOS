bits 32

global kernel_entry
extern kernel_main

kernel_entry:
    mov esp, 0x90000

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang