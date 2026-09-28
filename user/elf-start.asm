bits 64
section .rodata
global elf_start_code,elf_start_end
elf_start_code:
    ; Kernel supplies verified user addresses in callee-saved registers.
    sub rsp,8
    test r15,r15
    jz .array
    call r15
.array:
    test r13,r13
    jz .entry
    call [r12]
    add r12,8
    dec r13
    jmp .array
.entry:
    call r14
    mov rdi,rax
    mov eax,3
    int 0x80
    ud2
elf_start_end:
section .note.GNU-stack noalloc noexec nowrite progbits
