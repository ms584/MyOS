bits 64
default abs
org 0x8000000000
%include "user/syscall.inc"
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
    je unavailable_avx
    cmp rcx,14
    je syscall_contract
    cmp rcx,15
    je syscall_demo
    cmp rcx,16
    je syscall_negative_exit
    cmp rcx,17
    je fs_contract
    cmp rcx,18
    je fs_demo
    cmp rcx,19
    je fs_fault
    cmp rcx,20
    je fs_kill
    cmp rcx,21
    je fp_probe
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
unavailable_avx:
    vxorps ymm0,ymm0,ymm0 ; OSXSAVE stays disabled until extended state is supported.
    mov eax,SYS_EXIT
    xor edi,edi
    int 0x80
    ud2

fp_probe:
    sub rsp,544
    and rsp,-16
    fxsave64 [rsp]
    cmp word [rsp],0x37f
    jne .bad
    cmp byte [rsp+4],0
    jne .bad
    cmp dword [rsp+24],0x1f80
    jne .bad
%assign r 0
%rep 32
    cmp qword [rsp+160+r*8],0
    jne .bad
%assign r r+1
%endrep
    mov rdx,[rbx+16]
    mov [rsp+512],rdx
    fild qword [rsp+512]
    mov eax,edx
    and eax,3
    shl eax,10
    or eax,0x37f
    mov [rsp+520],ax
    fldcw [rsp+520]
    mov eax,edx
    and eax,3
    shl eax,13
    or eax,0x1f80
    mov [rsp+524],eax
    ldmxcsr [rsp+524]
%assign r 0
%rep 16
    movq xmm%+r,rdx
    punpcklqdq xmm%+r,xmm%+r
%assign r r+1
%endrep
.loop:
    fxsave64 [rsp]
    mov ax,[rsp+520]
    cmp [rsp],ax
    jne .bad
    mov eax,[rsp+524]
    cmp [rsp+24],eax
    jne .bad
%assign r 0
%rep 32
    cmp [rsp+160+r*8],rdx
    jne .bad
%assign r r+1
%endrep
    fld st0
    fistp qword [rsp+528]
    cmp [rsp+528],rdx
    jne .bad
    inc qword [rbx]
    cmp qword [rbx+64],0
    je .loop
    test qword [rbx],0x3ff
    jnz .loop
    inc qword [rbx+56]
    mov eax,SYS_YIELD
    int 0x80
    jmp .loop
.bad:
    mov qword [rbx+48],1
    ud2

%include "user/syscall-demo.inc"
%include "user/fs-demo.inc"
