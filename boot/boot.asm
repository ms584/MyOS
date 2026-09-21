bits 16
org 0x7c00

start:
    ; Set up segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Print message
    mov si, message

.print:
    lodsb

    ; End of string
    test al, al
    jz .hang

    ; BIOS teletype output
    mov ah, 0x0e
    mov bh, 0
    int 0x10

    jmp .print

.hang:
    cli
    hlt
    jmp .hang

message db "MyOS booting...", 13, 10, 0 

; Fill boot sector to 510 bytes
times 510 - ($ - $$) db 0

; Boot signature
dw 0xaa55