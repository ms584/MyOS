; Test-only SysV callee-save probe. Production jump asm is sealed musl.
bits 64
default rel
section .text
global MyOSJumpRegisterProbe:function
extern setjmp
extern longjmp
extern nanosleep
MyOSJumpRegisterProbe:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp,40
    mov [rsp],rdi
    mov [rsp+8],esi
    mov rbp,0x123456789abcdef0
    mov rbx,0x23456789abcdef01
    mov r12,0x3456789abcdef012
    mov r13,0x456789abcdef0123
    mov r14,0x56789abcdef01234
    mov r15,0x6789abcdef012345
    call setjmp wrt ..plt
    test eax,eax
    jnz .resumed
    xor ebp,ebp
    xor ebx,ebx
    xor r12d,r12d
    xor r13d,r13d
    xor r14d,r14d
    xor r15d,r15d
    mov qword [rsp+16],0
    mov qword [rsp+24],1
    lea rdi,[rsp+16]
    xor esi,esi
    call nanosleep wrt ..plt
    test eax,eax
    jnz .failed
    mov rdi,[rsp]
    mov esi,[rsp+8]
    call longjmp wrt ..plt
    ud2
.resumed:
    mov ecx,[rsp+8]
    test ecx,ecx
    jnz .nonzero
    inc ecx
.nonzero:
    cmp eax,ecx
    jne .failed
    mov rcx,0x123456789abcdef0
    cmp rbp,rcx
    jne .failed
    mov rcx,0x23456789abcdef01
    cmp rbx,rcx
    jne .failed
    mov rcx,0x3456789abcdef012
    cmp r12,rcx
    jne .failed
    mov rcx,0x456789abcdef0123
    cmp r13,rcx
    jne .failed
    mov rcx,0x56789abcdef01234
    cmp r14,rcx
    jne .failed
    mov rcx,0x6789abcdef012345
    cmp r15,rcx
    jne .failed
    mov eax,1
    jmp .done
.failed:
    xor eax,eax
.done:
    add rsp,40
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
