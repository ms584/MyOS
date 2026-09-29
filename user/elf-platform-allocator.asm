bits 64
global _start
extern __errno_location,aligned_alloc,calloc,free,malloc,malloc_usable_size,posix_memalign,realloc
section .text
_start:
    sub rsp,24
    mov edi,32
    call malloc wrt ..plt
    test rax,rax
    jz bad
    test rax,15
    jnz bad
    mov [rsp],rax
    mov byte [rax],0x5a
    mov rdi,rax
    call malloc_usable_size wrt ..plt
    cmp rax,32
    jb bad
    mov edi,4
    mov esi,8
    call calloc wrt ..plt
    test rax,rax
    jz bad
    mov [rsp+8],rax
    xor ecx,ecx
.zero:
    cmp byte [rax+rcx],0
    jne bad
    inc ecx
    cmp ecx,32
    jb .zero
    mov rdi,[rsp]
    mov esi,64
    call realloc wrt ..plt
    test rax,rax
    jz bad
    cmp byte [rax],0x5a
    jne bad
    mov [rsp],rax
    mov edi,64
    mov esi,128
    call aligned_alloc wrt ..plt
    test rax,rax
    jz bad
    test rax,63
    jnz bad
    mov [rsp+16],rax
    mov edi,64
    mov esi,65
    call aligned_alloc wrt ..plt
    test rax,rax
    jnz bad
    call __errno_location wrt ..plt
    cmp dword [rax],22
    jne bad
    mov qword [rel aligned],0
    lea rdi,[rel aligned]
    mov esi,128
    mov edx,33
    call posix_memalign wrt ..plt
    test eax,eax
    jnz bad
    mov rax,[rel aligned]
    test rax,127
    jnz bad
    mov rdi,rax
    call free wrt ..plt
    mov qword [rel aligned],0x11223344
    lea rdi,[rel aligned]
    mov esi,3
    mov edx,16
    call posix_memalign wrt ..plt
    cmp eax,22
    jne bad
    cmp qword [rel aligned],0x11223344
    jne bad
    mov rdi,-1
    mov esi,2
    call calloc wrt ..plt
    test rax,rax
    jnz bad
    call __errno_location wrt ..plt
    cmp dword [rax],12
    jne bad
    mov rdi,[rsp]
    call free wrt ..plt
    mov rdi,[rsp+8]
    call free wrt ..plt
    mov rdi,[rsp+16]
    call free wrt ..plt
    mov eax,1
    lea rdi,[rel message]
    mov esi,message_end-message
    int 0x80
    add rsp,24
    mov eax,33
    ret
bad:
    add rsp,24
    mov eax,99
    ret
section .data
aligned: dq 0
section .rodata
message: db 'ELF platform allocator semantics PASS',10
message_end:
section .note.GNU-stack noalloc noexec nowrite progbits
