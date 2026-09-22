bits 16
org 0x8000

stage2_start:
    xor ax, ax
    mov ds, ax

    cld

    ; Message located in first sector
    mov si, message
    call print_string

    ; Message deliberately located later
    ; in the Stage 2 image
    mov si, multi_sector_message
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; Print string
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


message:
    db "Stage 2 running!", 13, 10, 0


; ==================================
; Pad until offset 1536
;
; Stage2 sector 1 = offset    0
; Stage2 sector 2 = offset  512
; Stage2 sector 3 = offset 1024
; Stage2 sector 4 = offset 1536
; ==================================

times 1536 - ($ - $$) db 0

multi_sector_message:
    db "Multi-sector loading works!", 13, 10, 0


; ==================================
; Make Stage 2 exactly 4 sectors
; 4 * 512 = 2048 bytes
; ==================================

times 2048 - ($ - $$) db 0