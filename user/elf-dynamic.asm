bits 64
global _start,init_first
extern answer,exported_value
extern optional_hook:weak
section .text
init_first:
    mov qword [rel first],1
    ret
constructor:
    cmp qword [rel first],1
    jne .bad
    mov qword [rel second],2
    mov eax,1
    lea rdi,[rel ctor_message]
    mov esi,ctor_end-ctor_message
    int 0x80
.bad:
    ret
_start:
    cmp qword [rel first],1
    jne bad
    cmp qword [rel second],2
    jne bad
    mov rax,[rel exported_value wrt ..got]
    cmp qword [rax],332
    jne bad
    cmp rax,[rel absolute_pointer]
    jne bad
    mov rax,[rel relative_pointer]
    cmp qword [rax],123
    jne bad
    mov rax,[rel optional_hook wrt ..got]
    test rax,rax
    jnz bad
    sub rsp,8
    call answer wrt ..plt
    add rsp,8
    cmp eax,332
    jne bad
    mov eax,1
    lea rdi,[rel message]
    mov esi,message_end-message
    int 0x80
    mov eax,32
    ret
bad:
    mov eax,99
    ret
section .rodata
ctor_message: db 'ELF constructor executed in Ring 3',10
ctor_end:
message: db 'ELF dynamic: four relocations, weak binding and constructors PASS',10
message_end:
section .init_array write align=8
    dq constructor
section .data
absolute_pointer: dq exported_value
relative_pointer: dq local_value
local_value: dq 123
first: dq 0
second: dq 0
section .note.GNU-stack noalloc noexec nowrite progbits
