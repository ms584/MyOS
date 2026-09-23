bits 64
section .text
extern interrupt_dispatch
isr0:
    push qword 0
    push qword 0
    jmp interrupt_common
isr1:
    push qword 0
    push qword 1
    jmp interrupt_common
isr2:
    push qword 0
    push qword 2
    jmp interrupt_common
isr3:
    push qword 0
    push qword 3
    jmp interrupt_common
isr4:
    push qword 0
    push qword 4
    jmp interrupt_common
isr5:
    push qword 0
    push qword 5
    jmp interrupt_common
isr6:
    push qword 0
    push qword 6
    jmp interrupt_common
isr7:
    push qword 0
    push qword 7
    jmp interrupt_common
isr8:
    push qword 8
    jmp interrupt_common
isr9:
    push qword 0
    push qword 9
    jmp interrupt_common
isr10:
    push qword 10
    jmp interrupt_common
isr11:
    push qword 11
    jmp interrupt_common
isr12:
    push qword 12
    jmp interrupt_common
isr13:
    push qword 13
    jmp interrupt_common
isr14:
    push qword 14
    jmp interrupt_common
isr15:
    push qword 0
    push qword 15
    jmp interrupt_common
isr16:
    push qword 0
    push qword 16
    jmp interrupt_common
isr17:
    push qword 17
    jmp interrupt_common
isr18:
    push qword 0
    push qword 18
    jmp interrupt_common
isr19:
    push qword 0
    push qword 19
    jmp interrupt_common
isr20:
    push qword 0
    push qword 20
    jmp interrupt_common
isr21:
    push qword 21
    jmp interrupt_common
isr22:
    push qword 0
    push qword 22
    jmp interrupt_common
isr23:
    push qword 0
    push qword 23
    jmp interrupt_common
isr24:
    push qword 0
    push qword 24
    jmp interrupt_common
isr25:
    push qword 0
    push qword 25
    jmp interrupt_common
isr26:
    push qword 0
    push qword 26
    jmp interrupt_common
isr27:
    push qword 0
    push qword 27
    jmp interrupt_common
isr28:
    push qword 0
    push qword 28
    jmp interrupt_common
isr29:
    push qword 29
    jmp interrupt_common
isr30:
    push qword 30
    jmp interrupt_common
isr31:
    push qword 0
    push qword 31
    jmp interrupt_common
isr32:
    push qword 0
    push qword 32
    jmp interrupt_common
isr33:
    push qword 0
    push qword 33
    jmp interrupt_common
isr34:
    push qword 0
    push qword 34
    jmp interrupt_common
isr35:
    push qword 0
    push qword 35
    jmp interrupt_common
isr36:
    push qword 0
    push qword 36
    jmp interrupt_common
isr37:
    push qword 0
    push qword 37
    jmp interrupt_common
isr38:
    push qword 0
    push qword 38
    jmp interrupt_common
isr39:
    push qword 0
    push qword 39
    jmp interrupt_common
isr40:
    push qword 0
    push qword 40
    jmp interrupt_common
isr41:
    push qword 0
    push qword 41
    jmp interrupt_common
isr42:
    push qword 0
    push qword 42
    jmp interrupt_common
isr43:
    push qword 0
    push qword 43
    jmp interrupt_common
isr44:
    push qword 0
    push qword 44
    jmp interrupt_common
isr45:
    push qword 0
    push qword 45
    jmp interrupt_common
isr46:
    push qword 0
    push qword 46
    jmp interrupt_common
isr47:
    push qword 0
    push qword 47
    jmp interrupt_common
interrupt_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov rdi,rsp
    and rsp,-16
    call interrupt_dispatch
    mov rsp,rax
    mov ax,0x10
    test byte [rsp+144],3
    jz .segments
    mov ax,0x23
.segments:
    mov ds,ax
    mov es,ax
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp,16
    iretq
section .rodata
global isr_table
isr_table:
    dq isr0
    dq isr1
    dq isr2
    dq isr3
    dq isr4
    dq isr5
    dq isr6
    dq isr7
    dq isr8
    dq isr9
    dq isr10
    dq isr11
    dq isr12
    dq isr13
    dq isr14
    dq isr15
    dq isr16
    dq isr17
    dq isr18
    dq isr19
    dq isr20
    dq isr21
    dq isr22
    dq isr23
    dq isr24
    dq isr25
    dq isr26
    dq isr27
    dq isr28
    dq isr29
    dq isr30
    dq isr31
    dq isr32
    dq isr33
    dq isr34
    dq isr35
    dq isr36
    dq isr37
    dq isr38
    dq isr39
    dq isr40
    dq isr41
    dq isr42
    dq isr43
    dq isr44
    dq isr45
    dq isr46
    dq isr47
section .note.GNU-stack noalloc noexec nowrite progbits
