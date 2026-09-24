bits 64
section .text
global uefi_enter
uefi_enter:
    cli
    cld
    mov rbx,rdi
    mov ecx,0xc0000080
    rdmsr
    or eax,0x800
    wrmsr
    mov rax,cr4
    and rax,~0x80
    or rax,0x20
    mov cr4,rax
    mov rax,cr0
    and rax,~0xc
    or rax,0x10000
    mov cr0,rax
    lgdt [rel gdtr]
    push 0x18
    lea rax,[rel .code]
    push rax
    retfq
.code:
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov gs,ax
    mov cr3,rsi
    mov rdi,rbx
    mov rax,0x10000
    jmp rax
section .data
align 8
gdt: dq 0,0x00cf9a000000ffff,0x00cf92000000ffff,0x00af9a000000ffff
gdtr: dw 31
      dq gdt
section .note.GNU-stack noalloc noexec nowrite progbits
