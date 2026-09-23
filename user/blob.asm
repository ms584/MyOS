bits 64
section .rodata
global user_blob_start,user_blob_end
user_blob_start:
    incbin "build/user-demo.bin"
user_blob_end:
section .note.GNU-stack noalloc noexec nowrite progbits
