bits 64
section .rodata
global elf_start_code,elf_start_end
elf_start_code:
    ; Kernel supplies verified user addresses in callee-saved registers.
    ; Keep the finalizer count/status in our stack frame. rbx/rbp are ABI
    ; callee-saved; constructors and entry may clobber caller-saved registers.
    sub rsp,24
    mov [rsp],rdx
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
    mov [rsp+8],rax
    mov r13,[rsp]
.fini_array:
    test r13,r13
    jz .fini
    dec r13
    call [rbp+r13*8]
    jmp .fini_array
.fini:
    test rbx,rbx
    jz .exit
    call rbx
.exit:
    mov rdi,[rsp+8]
    mov eax,3
    int 0x80
    ud2
elf_start_end:
section .note.GNU-stack noalloc noexec nowrite progbits
