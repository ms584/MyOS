bits 64
section .rodata
global elf_start_code,elf_start_end
elf_start_code:
    ; Kernel supplies verified user addresses in callee-saved registers.
    ; Keep the finalizer count/status in our stack frame. rbx/rbp are ABI
    ; callee-saved; constructors and entry may clobber caller-saved registers.
    sub rsp,88
    mov [rsp+72],r11
    mov [rsp+40],rbx
    mov [rsp+48],rbp
    mov [rsp],rdx
    mov [rsp+16],rdi
    mov [rsp+24],rsi
    mov [rsp+32],rcx
    mov [rsp+56],r10
    ; SABI validation runs in Ring 3 before any constructor. A rejected or
    ; faulting getter cannot reach application initialization/finalizers.
    test r8,r8
    jz .init
    mov rdi,r9
    call r8
    mov [rsp+8],rax
    test rax,rax
    jnz .exit
.init:
    ; Publish the owned frame before constructors may call SDK exit().
    mov r11,[rsp+72]
    test r11,r11
    jz .init_call
    mov [r11+56],rsp
    lea rax,[rel .returned]
    mov [rsp+64],rax
.init_call:
    test r15,r15
    jz .array
    call r15
.array:
    test r13,r13
    jz .entry
    call [r12]
    add r12,8
    dec r13
    jmp .array
.entry:
.call_entry:
    mov rdi,[rsp+16]
    mov rsi,[rsp+24]
    mov rdx,[rsp+32]
    mov rcx,[rsp+56]
    call r14
.returned:
    mov [rsp+8],rax
    mov r13,[rsp]
.fini_array:
    test r13,r13
    jz .fini
    dec r13
    mov [rsp],r13
    call [rbp+r13*8]
    jmp .fini_array
.fini:
    test rbx,rbx
    jz .exit
    mov qword [rsp+40],0
    call rbx
.exit:
    mov rdi,[rsp+8]
    mov eax,3
    int 0x80
    ud2
elf_start_end:
section .note.GNU-stack noalloc noexec nowrite progbits
