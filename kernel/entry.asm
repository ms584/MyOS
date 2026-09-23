bits 64
section .text.entry
global kernel_entry
extern kernel_main, __bss_start, __bss_end
kernel_entry:
    cli
    cld
    mov rsp, 0x90000
    xor eax, eax
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    rep stosb
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
section .note.GNU-stack noalloc noexec nowrite progbits
