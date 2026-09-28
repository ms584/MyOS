bits 64
global answer,exported_value
section .text
answer: mov eax,332
ret
section .data
exported_value: dq 332
section .note.GNU-stack noalloc noexec nowrite progbits
