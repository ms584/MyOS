; Appended to the embedded user image; entered by its mode dispatch.
syscall_demo:
    mov eax,SYS_GETPID
    int 0x80
    mov [rbx+48],rax
    mov eax,SYS_WRITE
    mov rdi,hello_message
    mov esi,hello_end-hello_message
    int 0x80
    cmp rax,hello_end-hello_message
    jne syscall_failed
    mov eax,SYS_YIELD
    int 0x80
    test rax,rax
    jne syscall_failed
    mov eax,SYS_EXIT
    mov edi,42
    int 0x80
    ud2

syscall_negative_exit:
    mov eax,SYS_EXIT
    mov edi,-7
    int 0x80
    ud2

%macro expect_result 1
    cmp rax,%1
    jne syscall_failed
    inc qword [rbx+56]
%endmacro

syscall_contract:
    ; INT must preserve non-result registers, IF, and the user's DF.
    mov rcx,0x1234
    mov r11,0x5678
    mov r12,0xabcd
    mov rdi,0x1122
    mov rsi,0x3344
    mov rdx,0x5566
    std
    mov eax,SYS_GETPID
    int 0x80
    mov [rbx+48],rax
    test rax,rax
    jz syscall_failed
    cmp rcx,0x1234
    jne syscall_failed
    cmp r11,0x5678
    jne syscall_failed
    cmp r12,0xabcd
    jne syscall_failed
    cmp rdi,0x1122
    jne syscall_failed
    cmp rsi,0x3344
    jne syscall_failed
    cmp rdx,0x5566
    jne syscall_failed
    pushfq
    pop rax
    and eax,0x3600
    cmp eax,0x600
    jne syscall_failed
    cld
    inc qword [rbx+56]

    mov eax,0xffffffff
    int 0x80
    expect_result -38

    mov eax,SYS_WRITE
    mov edi,0x10000
    mov esi,8
    int 0x80
    expect_result -14

    mov eax,SYS_WRITE
    mov rdi,0x100000000
    int 0x80
    expect_result -14

    mov eax,SYS_WRITE
    mov rdi,0x0000800000000000
    int 0x80
    expect_result -14

    mov eax,SYS_WRITE
    mov rdi,0xfffffffffffffffc
    int 0x80
    expect_result -14

    ; Last bytes of valid data followed by an unmapped guard: no partial output.
    mov eax,SYS_WRITE
    mov rdi,0x8000001ffc
    int 0x80
    expect_result -14

    mov eax,SYS_WRITE
    mov rdi,0x8000002000
    mov esi,1
    int 0x80
    expect_result -14

    mov eax,SYS_WRITE
    mov rdi,hello_message
    mov esi,257
    int 0x80
    expect_result -22

    mov eax,SYS_WRITE
    mov rdi,-1
    xor esi,esi
    int 0x80
    expect_result 0

    ; Both code-page reads and buffers crossing readable pages are supported.
    mov eax,SYS_WRITE
    mov rdi,rx_message
    mov esi,rx_end-rx_message
    int 0x80
    expect_result rx_end-rx_message

    mov eax,SYS_WRITE
    mov rdi,0x8000000ffe
    mov esi,4
    int 0x80
    expect_result 4

    sub rsp,16
    mov dword [rsp],0x000a4b4f
    mov rdi,rsp
    mov esi,3
    mov eax,SYS_WRITE
    int 0x80
    add rsp,16
    expect_result 3

    mov r13d,16
.yield:
    mov eax,SYS_YIELD
    int 0x80
    expect_result 0
    dec r13d
    jnz .yield
    mov eax,SYS_EXIT
    xor edi,edi
    int 0x80
    ud2

syscall_failed:
    cld
    mov eax,SYS_EXIT
    mov edi,200
    int 0x80
    ud2
hello_message: db 'Hello from Ring 3 via sys_write!',10
hello_end:
rx_message: db 'SYS_TEST_RX',10
rx_end:
