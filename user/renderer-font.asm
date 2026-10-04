bits 64
section .rodata progbits alloc noexec nowrite
global renderer_font
renderer_font: incbin "assets/myos-font.bin"
section .note.GNU-stack noalloc noexec nowrite progbits
