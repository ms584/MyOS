; Phase 11 file API. All pointers are user addresses; no kernel memory access.
fs_demo:
    mov eax,SYS_FOPEN
    mov rdi,fs_motd
    mov esi,1
    int 0x80
    cmp rax,3
    jl fs_fail
    mov r12,rax
    mov eax,SYS_FREAD
    mov rdi,r12
    lea rsi,[rbx+256]
    mov edx,128
    int 0x80
    cmp rax,23
    jne fs_fail
    mov rsi,rax
    lea rdi,[rbx+256]
    mov eax,SYS_WRITE
    int 0x80
    mov eax,SYS_FCLOSE
    mov rdi,r12
    int 0x80
    test rax,rax
    jnz fs_fail
    jmp fs_ok

%macro FS_EXPECT 1
    cmp rax,%1
    jne fs_fail
    inc qword [rbx+56]
%endmacro
fs_contract:
    mov eax,SYS_FOPEN
    mov rdi,fs_file
    mov esi,7
    int 0x80
    FS_EXPECT 3
    mov eax,SYS_FWRITE
    mov edi,3
    mov rsi,fs_payload
    mov edx,4
    int 0x80
    FS_EXPECT 4
    mov eax,SYS_FOPEN
    mov rdi,fs_file
    mov esi,1
    int 0x80
    FS_EXPECT 4
    ; Rejected copyout into RX code and across the unmapped guard must not
    ; advance the descriptor offset or write a partial user buffer.
    mov eax,SYS_FREAD
    mov edi,4
    mov rsi,fs_payload
    mov edx,4
    int 0x80
    FS_EXPECT -14
    mov byte [rbx+4095],0x5a
    mov eax,SYS_FREAD
    mov edi,4
    lea rsi,[rbx+4095]
    mov edx,2
    int 0x80
    FS_EXPECT -14
    cmp byte [rbx+4095],0x5a
    jne fs_fail
    mov eax,SYS_FREAD
    mov edi,4
    lea rsi,[rbx+256]
    mov edx,4
    int 0x80
    FS_EXPECT 4
    cmp dword [rbx+256],0x44434241
    jne fs_fail
    mov eax,SYS_FWRITE
    mov edi,3
    lea rsi,[rbx+4095]
    mov edx,2
    int 0x80
    FS_EXPECT -14
    mov eax,SYS_FWRITE
    mov edi,4
    mov rsi,fs_payload
    mov edx,1
    int 0x80
    FS_EXPECT -9
    mov eax,SYS_FOPEN
    mov edi,0x10000
    mov esi,1
    int 0x80
    FS_EXPECT -14
    mov eax,SYS_FREAD
    mov rdi,0x100000004
    lea rsi,[rbx+256]
    mov edx,1
    int 0x80
    FS_EXPECT -9
    mov eax,SYS_FREAD
    mov edi,4
    lea rsi,[rbx+256]
    mov edx,257
    int 0x80
    FS_EXPECT -22
    mov eax,SYS_STAT
    mov rdi,fs_file
    lea rsi,[rbx+256]
    int 0x80
    FS_EXPECT 0
    cmp qword [rbx+296],4
    jne fs_fail
    mov eax,SYS_READDIR
    mov rdi,fs_home
    lea rsi,[rbx+256]
    xor edx,edx
    int 0x80
    FS_EXPECT 1
    mov eax,SYS_MKDIR
    mov rdi,fs_dir
    int 0x80
    FS_EXPECT 0
    mov eax,SYS_MKDIR
    mov rdi,fs_dir
    int 0x80
    FS_EXPECT -17
    mov eax,SYS_UNLINK
    mov rdi,fs_dir
    int 0x80
    FS_EXPECT 0
    mov eax,SYS_FCLOSE
    mov edi,4
    int 0x80
    FS_EXPECT 0
    mov eax,SYS_FCLOSE
    mov edi,4
    int 0x80
    FS_EXPECT -9
    ; Leave descriptor 3 open: exit must close it before reap.
fs_ok:
    mov eax,SYS_EXIT
    xor edi,edi
    int 0x80
    ud2
fs_fault:
    mov eax,SYS_FOPEN
    mov rdi,fs_file
    mov esi,7
    int 0x80
    FS_EXPECT 3
    ud2
fs_kill:
    mov eax,SYS_FOPEN
    mov rdi,fs_file
    mov esi,7
    int 0x80
    FS_EXPECT 3
.wait:
    inc qword [rbx]
    jmp .wait
fs_fail:
    mov eax,SYS_EXIT
    mov edi,99
    int 0x80
    ud2
fs_motd: db '/etc/motd',0
fs_file: db '/home/user.txt',0
fs_home: db '/home',0
fs_dir: db '/home/userdir',0
fs_payload: db 'ABCD'
