bits 16
org 0x7c00

start:
    ; Set up segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Print boot message
    mov si, message

.print:
    lodsb

    ; End of string
    test al, al
    jz .wait_key

    ; BIOS teletype output
    mov ah, 0x0e
    mov bh, 0
    int 0x10

    jmp .print


.wait_key:
    ; Print prompt
    mov si, prompt

.print_prompt:
    lodsb

    test al, al
    jz .read_key

    mov ah, 0x0e
    mov bh, 0
    int 0x10

    jmp .print_prompt


.read_key:
    ; BIOS keyboard input
    ; AH = 00h
    ; INT 16h waits for a key
    mov ah, 0x00
    int 0x16

    ; AL contains ASCII character

    ; Print the pressed key
    mov ah, 0x0e
    mov bh, 0
    int 0x10

    ; Wait for another key
    jmp .read_key


message db "MyOS booting...", 13, 10, 0
prompt  db "Press a key: ", 0

; Fill the boot sector to 510 bytes
times 510 - ($ - $$) db 0

; Boot signature
dw 0xaa55