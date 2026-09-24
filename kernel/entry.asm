bits 64
section .text.entry
global kernel_entry
extern kernel_main, boot_init, __bss_start, __bss_end
kernel_entry:
    cli
    cld
    mov r12,rdi
    mov rsp,0x90000
    test rdi,rdi
    jz .stack_ready
    mov rsp,[rdi+16]
.stack_ready:
    xor eax, eax
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    rep stosb
    mov rdi,r12
    call boot_init
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
section .note.GNU-stack noalloc noexec nowrite progbits
