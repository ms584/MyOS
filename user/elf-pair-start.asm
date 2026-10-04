bits 64
section .rodata
global elf_pair_start_code,elf_pair_start_end
elf_pair_start_code:
    ; rax: RW/NX lifecycle control; r15: shared caller-stack finalizer.
    sub rsp,56
    mov [rsp],rdi
    mov [rsp+8],rsi
    mov [rsp+16],rcx
    mov [rsp+24],r10
    mov [rsp+32],rax
    mov [rsp+40],r11
    test r8,r8
    jz .validated
    mov rdi,r9
    call r8
    test rax,rax
    jnz .exit
.validated:
    mov rax,[rsp+32]
    mov r11,[rsp+40]
    mov [r11+56],rax
    lea r12,[rax+96]
    call .initialize
    mov r12,[rsp+32]
    add r12,32
    call .initialize
    mov rdi,[rsp]
    mov rsi,[rsp+8]
    mov rdx,[rsp+16]
    mov rcx,[rsp+24]
    call r14
    mov [rsp+48],rax
    call r15
    mov rax,[rsp+48]
.exit:
    mov rdi,rax
    mov eax,3
    int 0x80
    ud2
.initialize:
    ; Inner call enters with rsp%16=8; align before calling image functions.
    sub rsp,8
    mov qword [r12+56],1
    mov rax,[r12]
    test rax,rax
    jz .array
    call rax
.array:
    xor r13d,r13d
.next:
    cmp r13,[r12+16]
    jae .done
    mov rax,[r12+8]
    call [rax+r13*8]
    inc r13
    jmp .next
.done:
    add rsp,8
    ret
elf_pair_start_end:
section .note.GNU-stack noalloc noexec nowrite progbits
