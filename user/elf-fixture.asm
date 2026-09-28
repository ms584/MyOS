bits 64
global _start
section .text
_start:
    ; Check initialized data, multi-page BSS and read-only payload above 64 KiB.
    cmp qword [rel initial],331
    jne bad
    cmp byte [rel padding_end-1],0x5a
    jne bad
    cmp qword [rel zeroes],0
    jne bad
    cmp qword [rel zeroes+12280],0
    jne bad
    mov qword [rel zeroes+12280],123
    mov eax,1
    lea rdi,[rel message]
    mov esi,message_end-message
    int 0x80
    mov edi,33
    jmp done
bad:
    mov edi,99
done:
    mov eax,3
    int 0x80
    ud2
section .rodata
message: db 'ELF Ring 3: data, BSS and >64 KiB asset PASS',10
message_end:
times 70000 db 0x5a
padding_end:
section .data
initial: dq 331
section .bss
zeroes: resb 12288
section .note.GNU-stack noalloc noexec nowrite progbits
