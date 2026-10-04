bits 64
section .text
global MyOSSignalReturn
MyOSSignalReturn:
    ; A handler RET leaves the user-address token above its return address.
    ; Kernel restores only the matching top kernel-owned GP/flags/FP context.
    mov rsi,[rsp]
    mov edi,3
    mov eax,49
    int 0x80
    ud2                         ; rejected/tampered return faults this process
global MyOSTlsDesc
MyOSTlsDesc:
    ; x64 TLSDESC ABI: descriptor in rax, return TP-relative offset in rax.
    ; Preserve every other GP/SIMD register (including argument registers).
    mov rax,[rax+8]
    ret
global MyOSMainThreadExit
MyOSMainThreadExit:
    ; Resume only the loader-owned main frame, with a zero process status.
    ; The abandoned application frames and their register choices are ignored.
    mov rsp,rdi
    mov rbx,[rsp+40]
    mov rbp,[rsp+48]
    xor eax,eax
    jmp [rsp+64]
section .note.GNU-stack noalloc noexec nowrite progbits
