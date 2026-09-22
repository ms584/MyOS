bits 16
org 0x8000

stage2_start:
    ; DS = 0 so labels using org 0x8000 work correctly
    xor ax, ax
    mov ds, ax

    cld

    mov si, message
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; -------------------------
; Print null-terminated string
; DS:SI -> string
; -------------------------
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


message db "Stage 2 loaded successfully!", 13, 10, 0


; Make Stage 2 exactly one sector
times 512 - ($ - $$) db 0
