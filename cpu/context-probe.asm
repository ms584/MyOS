bits 64
section .text
global context_probe
extern panic
context_probe:
    push rdi
    mov rax,1192960
    mov rbx,1192961
    mov rcx,1192962
    mov rdx,1192963
    mov rbp,1192964
    mov rsi,1192965
    mov rdi,1192966
    mov r8,1192967
    mov r9,1192968
    mov r10,1192969
    mov r11,1192970
    mov r12,1192971
    mov r13,1192972
    mov r14,1192973
    mov r15,1192974
    std
.loop:
    cmp rax,1192960
    jne .fail
    cmp rbx,1192961
    jne .fail
    cmp rcx,1192962
    jne .fail
    cmp rdx,1192963
    jne .fail
    cmp rbp,1192964
    jne .fail
    cmp rsi,1192965
    jne .fail
    cmp rdi,1192966
    jne .fail
    cmp r8,1192967
    jne .fail
    cmp r9,1192968
    jne .fail
    cmp r10,1192969
    jne .fail
    cmp r11,1192970
    jne .fail
    cmp r12,1192971
    jne .fail
    cmp r13,1192972
    jne .fail
    cmp r14,1192973
    jne .fail
    cmp r15,1192974
    jne .fail
    pushfq
    pop rax
    test rax,0x400
    jz .fail
    mov rax,[rsp]
    inc qword [rax]
    mov rax,0x123400
    jmp .loop
.fail:
    cld
    lea rdi,[rel message]
    call panic
section .rodata
message: db 'context register corruption',0
section .note.GNU-stack noalloc noexec nowrite progbits
