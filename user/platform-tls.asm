bits 64
section .text
global MyOSTlsDesc
MyOSTlsDesc:
    ; x64 TLSDESC ABI: descriptor in rax, return TP-relative offset in rax.
    ; Preserve every other GP/SIMD register (including argument registers).
    mov rax,[rax+8]
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
