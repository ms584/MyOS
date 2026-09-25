bits 64
section .rodata
global hello_app_start,hello_app_end
hello_app_start: incbin "build/hello.app"
hello_app_end:
section .note.GNU-stack noalloc noexec nowrite progbits
