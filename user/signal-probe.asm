bits 64
default rel
section .text
global MyOSSignalProbe
global MyOSSignalClobber
extern signal_probe_count
MyOSSignalProbe:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp,32
    stmxcsr [rsp+24]
    fnstcw [rsp+28]
    ldmxcsr [initial_mxcsr]
    fldcw [initial_cw]
    fld1
%assign i 0
%rep 16
    movdqu xmm%+i,[pattern]
%assign i i+1
%endrep
    mov rbp,0x1212
    mov rbx,0x1313
    mov r12,0x1414
    mov r13,0x1515
    mov r14,0x1616
    mov r15,0x1717
    mov r8,0x1818
    mov r9,0x1919
    mov r10,0x2020
    mov r11,0x2121
    mov rcx,0x2222
    mov rdx,0x2323
    mov edi,2
    mov esi,10
    mov eax,49
    std
    pushfq
    pop qword [rsp+16]
    int 0x80
    pushfq
    pop qword [rsp+8]
    ; Handler deliberately overwrites GP/FP. Only kernel-owned restoration
    ; can preserve every one of these values, including DF and x87 state.
    cmp rax,0
    jne .failed
    mov rax,[rsp+8]
    cmp rax,[rsp+16]
    jne .failed
    test eax,1<<10
    jz .failed
    cld
%macro VERIFY 2
    cmp %1,%2
    jne .failed
%endmacro
    VERIFY rbp,0x1212
    VERIFY rbx,0x1313
    VERIFY r12,0x1414
    VERIFY r13,0x1515
    VERIFY r14,0x1616
    VERIFY r15,0x1717
    VERIFY r8,0x1818
    VERIFY r9,0x1919
    VERIFY r10,0x2020
    VERIFY r11,0x2121
    VERIFY rcx,0x2222
    VERIFY rdx,0x2323
    VERIFY rdi,2
    VERIFY rsi,10
%assign i 0
%rep 16
    pcmpeqb xmm%+i,[pattern]
    pmovmskb eax,xmm%+i
    cmp eax,65535
    jne .failed
%assign i i+1
%endrep
    fstp tword [rsp]
    mov rax,0x8000000000000000
    cmp [rsp],rax
    jne .failed
    cmp word [rsp+8],0x3fff
    jne .failed
    stmxcsr [rsp+12]
    cmp dword [rsp+12],0x3f80
    jne .failed
    fnstcw [rsp+16]
    cmp word [rsp+16],0x077f
    jne .failed
    xor eax,eax
    jmp .done
.failed:
    cld
    fninit
    mov eax,1
.done:
    ldmxcsr [rsp+24]
    fldcw [rsp+28]
    add rsp,32
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
MyOSSignalClobber:
    pushfq
    pop rax
    test eax,1<<10
    jnz .bad_df
    inc qword [signal_probe_count]
    ldmxcsr [changed_mxcsr]
    fldcw [changed_cw]
    fldz
%assign i 0
%rep 16
    pxor xmm%+i,xmm%+i
%assign i i+1
%endrep
    xor ebp,ebp
    xor ebx,ebx
    xor r12d,r12d
    xor r13d,r13d
    xor r14d,r14d
    xor r15d,r15d
    xor r8d,r8d
    xor r9d,r9d
    xor r10d,r10d
    xor r11d,r11d
    xor ecx,ecx
    xor edx,edx
    xor edi,edi
    xor esi,esi
    mov eax,4                   ; preserve interrupted FP across a real switch
    int 0x80
    mov eax,0x3131
    ret
.bad_df:
    ud2
section .rodata align=16
pattern: dq 0x123456789abcdef0,0x0fedcba9876543210
initial_mxcsr: dd 0x3f80
changed_mxcsr: dd 0x7f80
initial_cw: dw 0x077f
changed_cw: dw 0x0f7f
section .note.GNU-stack noalloc noexec nowrite progbits
