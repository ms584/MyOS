bits 64
global _start
extern __errno_location,madvise,mincore,mmap,mprotect,msync,munmap
%define SYS_YIELD 4
%define SYS_VM_MAP 14
%define SYS_VM_UNMAP 15
%define SYS_THREAD_CREATE 17
%define SYS_THREAD_EXIT 18
%define SYS_THREAD_REAP 19
section .text
_start:
    sub rsp,40
    call __errno_location wrt ..plt
    test rax,rax
    jz bad
    mov [rel main_errno],rax
    mov dword [rax],0
    xor edi,edi
    mov esi,8192
    mov edx,3
    mov ecx,0x22
    mov r8d,-1
    xor r9d,r9d
    call mmap wrt ..plt
    cmp rax,-1
    je bad
    mov [rsp],rax
    cmp qword [rax],0
    jne bad
    cmp qword [rax+4096],0
    jne bad
    mov qword [rax],0x11223344
    mov byte [rsp+24],0
    mov byte [rsp+25],0
    mov rdi,rax
    mov esi,8192
    lea rdx,[rsp+24]
    call mincore wrt ..plt
    test eax,eax
    jnz bad
    cmp word [rsp+24],0x0101
    jne bad
    mov rdi,[rsp]
    mov esi,8192
    mov edx,4
    call madvise wrt ..plt
    test eax,eax
    jnz bad
    mov rax,[rsp]
    cmp qword [rax],0
    jne bad
    mov rdi,rax
    mov esi,8192
    mov edx,4
    call msync wrt ..plt
    test eax,eax
    jnz bad
    mov rdi,[rsp]
    mov esi,4096
    xor edx,edx
    call mprotect wrt ..plt
    test eax,eax
    jnz bad
    mov rdi,[rsp]
    mov esi,4096
    mov edx,3
    call mprotect wrt ..plt
    test eax,eax
    jnz bad
    mov rdi,[rsp]
    mov esi,4096
    mov edx,3
    mov ecx,0x32
    mov r8d,-1
    xor r9d,r9d
    call mmap wrt ..plt
    cmp rax,[rsp]
    jne bad
    cmp qword [rax],0
    jne bad
    mov rdi,rax
    mov esi,4096
    mov edx,3
    mov ecx,0x100022
    mov r8d,-1
    xor r9d,r9d
    call mmap wrt ..plt
    cmp rax,-1
    jne bad
    call __errno_location wrt ..plt
    cmp dword [rax],17
    jne bad
    mov rdi,[rsp]
    add rdi,4096
    mov esi,1
    call munmap wrt ..plt
    test eax,eax
    jnz bad
    mov byte [rsp+24],0
    mov rdi,[rsp]
    add rdi,4096
    mov esi,1
    lea rdx,[rsp+24]
    call mincore wrt ..plt
    cmp eax,-1
    jne bad
    call __errno_location wrt ..plt
    cmp dword [rax],12
    jne bad
    xor edi,edi
    mov esi,4096
    mov edx,3
    mov ecx,0x20
    mov r8d,-1
    xor r9d,r9d
    call mmap wrt ..plt
    cmp rax,-1
    jne bad
    call __errno_location wrt ..plt
    cmp dword [rax],22
    jne bad
    mov eax,SYS_VM_MAP
    xor edi,edi
    mov esi,4096
    mov edx,3
    int 0x80
    test rax,rax
    js bad
    mov [rsp+8],rax
    mov eax,SYS_THREAD_CREATE
    lea rdi,[rel worker]
    lea rsi,[rel worker_errno]
    mov rdx,[rsp+8]
    add rdx,4096
    xor r10d,r10d
    int 0x80
    test rax,rax
    js bad
    mov [rsp+16],rax
.reap:
    mov eax,SYS_THREAD_REAP
    mov rdi,[rsp+16]
    lea rsi,[rsp+28]
    int 0x80
    cmp rax,-11
    jne .reaped
    mov eax,SYS_YIELD
    int 0x80
    jmp .reap
.reaped:
    test eax,eax
    jnz bad
    cmp dword [rsp+28],44
    jne bad
    mov rax,[rel worker_errno]
    test rax,rax
    jz bad
    cmp rax,[rel main_errno]
    je bad
    cmp dword [rax],77
    jne bad
    mov rax,[rel main_errno]
    cmp dword [rax],22
    jne bad
    mov eax,SYS_VM_UNMAP
    mov rdi,[rsp+8]
    mov esi,4096
    int 0x80
    test eax,eax
    jnz bad
    mov rdi,[rsp]
    mov esi,4096
    call munmap wrt ..plt
    test eax,eax
    jnz bad
    mov eax,1
    lea rdi,[rel message]
    mov esi,message_end-message
    int 0x80
    add rsp,40
    mov eax,34
    ret
worker:
    mov rbx,rdi
    sub rsp,8
    call __errno_location wrt ..plt
    add rsp,8
    mov [rbx],rax
    mov dword [rax],77
    mov eax,SYS_THREAD_EXIT
    mov edi,44
    int 0x80
    ud2
bad:
    add rsp,40
    mov eax,99
    ret
section .data
main_errno: dq 0
worker_errno: dq 0
section .rodata
message: db 'ELF POSIX VM and per-thread errno PASS',10
message_end:
section .note.GNU-stack noalloc noexec nowrite progbits
