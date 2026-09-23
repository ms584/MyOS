bits 16
org 0x0000

KERNEL_SEGMENT equ 0x1000

kernel_start:
    ; Kernel is executing at 1000:0000

    mov ax, KERNEL_SEGMENT
    mov ds, ax
    mov es, ax

    cld

    mov si, kernel_message
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; Print null-terminated string
; DS:SI -> string
; ==================================

print_string:
.next:
    lodsb

    test al, al
    jz .done

    mov ah, 0x0e
    mov bh, 0x00
    int 0x10

    jmp .next

.done:
    ret


kernel_message:
    db "Hello from MyOS Kernel!", 13, 10, 0


; Kernel = 4 sectors
times 2048 - ($ - $$) db 0