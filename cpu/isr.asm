bits 32
section .text
extern exception_handler
; All handlers are fatal. No IRQs, user-mode transitions, or recovery yet.
isr0:
    push dword 0
    push dword 0
    jmp isr_common
isr1:
    push dword 0
    push dword 1
    jmp isr_common
isr2:
    push dword 0
    push dword 2
    jmp isr_common
isr3:
    push dword 0
    push dword 3
    jmp isr_common
isr4:
    push dword 0
    push dword 4
    jmp isr_common
isr5:
    push dword 0
    push dword 5
    jmp isr_common
isr6:
    push dword 0
    push dword 6
    jmp isr_common
isr7:
    push dword 0
    push dword 7
    jmp isr_common
isr8:
    push dword 8
    jmp isr_common
isr9:
    push dword 0
    push dword 9
    jmp isr_common
isr10:
    push dword 10
    jmp isr_common
isr11:
    push dword 11
    jmp isr_common
isr12:
    push dword 12
    jmp isr_common
isr13:
    push dword 13
    jmp isr_common
isr14:
    push dword 14
    jmp isr_common
isr15:
    push dword 0
    push dword 15
    jmp isr_common
isr16:
    push dword 0
    push dword 16
    jmp isr_common
isr17:
    push dword 17
    jmp isr_common
isr18:
    push dword 0
    push dword 18
    jmp isr_common
isr19:
    push dword 0
    push dword 19
    jmp isr_common
isr20:
    push dword 0
    push dword 20
    jmp isr_common
isr21:
    push dword 21
    jmp isr_common
isr22:
    push dword 0
    push dword 22
    jmp isr_common
isr23:
    push dword 0
    push dword 23
    jmp isr_common
isr24:
    push dword 0
    push dword 24
    jmp isr_common
isr25:
    push dword 0
    push dword 25
    jmp isr_common
isr26:
    push dword 0
    push dword 26
    jmp isr_common
isr27:
    push dword 0
    push dword 27
    jmp isr_common
isr28:
    push dword 0
    push dword 28
    jmp isr_common
isr29:
    push dword 29
    jmp isr_common
isr30:
    push dword 30
    jmp isr_common
isr31:
    push dword 0
    push dword 31
    jmp isr_common
isr_common:
    cld
    pushad
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    and esp, -16
    sub esp, 12
    push eax
    call exception_handler
.halt:
    cli
    hlt
    jmp .halt
section .rodata
global isr_table
isr_table:
    dd isr0
    dd isr1
    dd isr2
    dd isr3
    dd isr4
    dd isr5
    dd isr6
    dd isr7
    dd isr8
    dd isr9
    dd isr10
    dd isr11
    dd isr12
    dd isr13
    dd isr14
    dd isr15
    dd isr16
    dd isr17
    dd isr18
    dd isr19
    dd isr20
    dd isr21
    dd isr22
    dd isr23
    dd isr24
    dd isr25
    dd isr26
    dd isr27
    dd isr28
    dd isr29
    dd isr30
    dd isr31
section .note.GNU-stack noalloc noexec nowrite progbits
