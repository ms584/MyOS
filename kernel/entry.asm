bits 32
section .text.entry
global kernel_entry
extern kernel_main
extern __bss_start
extern __bss_end

kernel_entry:
    cli
    cld
    mov esp, 0x90000

    ; Flat binary loading does not initialize ELF NOBITS sections.
    xor eax, eax
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    rep stosb

    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
