bits 32
section .text
extern irq_handler
irq0:
    push dword 0
    push dword 32
    jmp irq_common
irq1:
    push dword 0
    push dword 33
    jmp irq_common
irq2:
    push dword 0
    push dword 34
    jmp irq_common
irq3:
    push dword 0
    push dword 35
    jmp irq_common
irq4:
    push dword 0
    push dword 36
    jmp irq_common
irq5:
    push dword 0
    push dword 37
    jmp irq_common
irq6:
    push dword 0
    push dword 38
    jmp irq_common
irq7:
    push dword 0
    push dword 39
    jmp irq_common
irq8:
    push dword 0
    push dword 40
    jmp irq_common
irq9:
    push dword 0
    push dword 41
    jmp irq_common
irq10:
    push dword 0
    push dword 42
    jmp irq_common
irq11:
    push dword 0
    push dword 43
    jmp irq_common
irq12:
    push dword 0
    push dword 44
    jmp irq_common
irq13:
    push dword 0
    push dword 45
    jmp irq_common
irq14:
    push dword 0
    push dword 46
    jmp irq_common
irq15:
    push dword 0
    push dword 47
    jmp irq_common
irq_common:
    pushad
    push ds
    push es
    push fs
    push gs
    cld
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebx, esp
    and esp, -16
    sub esp, 12
    push dword [ebx + 48]
    call irq_handler
    mov esp, ebx
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8
    iretd
section .rodata
global irq_table
irq_table:
    dd irq0
    dd irq1
    dd irq2
    dd irq3
    dd irq4
    dd irq5
    dd irq6
    dd irq7
    dd irq8
    dd irq9
    dd irq10
    dd irq11
    dd irq12
    dd irq13
    dd irq14
    dd irq15
section .note.GNU-stack noalloc noexec nowrite progbits
