bits 64
default abs
org 0x8000000000
    mov rbx,0x8000001000
    xor eax,eax
    mov ax,cs
    mov [rbx+8],rax
    mov rax,[rbx+16]
    push rax
    pop r8
    mov [rbx+32],r8
    mov [rbx+40],rsp
    mov rcx,[rbx+24]
    cmp rcx,1
    je read_kernel
    cmp rcx,2
    je write_kernel
    cmp rcx,3
    je privileged_cli
    cmp rcx,4
    je privileged_io
    cmp rcx,5
    je write_code
    cmp rcx,6
    je execute_stack
    cmp rcx,7
    je read_guard
    cmp rcx,8
    je invalid_opcode
    cmp rcx,9
    je privileged_interrupt
    cmp rcx,10
    je read_heap
    cmp rcx,11
    je read_cr3
    cmp rcx,12
    je execute_kernel
    cmp rcx,13
    je unavailable_fpu
.loop:
    inc qword [rbx]
    jmp .loop
read_kernel:
    mov rax,[0x10000]
    ud2
write_kernel:
    mov qword [0x10000],0
    ud2
privileged_cli:
    cli
    ud2
privileged_io:
    out 0x80,al
    ud2
write_code:
    mov rax,0x8000000000
    mov byte [rax],0x90
    ud2
execute_stack:
    mov rax,0x8000003000
    jmp rax
read_guard:
    mov rax,0x8000002000
    mov rax,[rax]
    ud2
invalid_opcode:
    ud2
privileged_interrupt:
    int 0x20
    ud2
read_heap:
    mov rax,0x100000000
    mov rax,[rax]
    ud2
read_cr3:
    mov rax,cr3
    ud2
execute_kernel:
    mov rax,0x10000
    jmp rax
unavailable_fpu:
    fnop
    ud2
