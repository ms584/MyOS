bits 16
org 0x7c00

start:
    ; -------------------------
    ; Setup segments
    ; -------------------------
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    cld

    ; BIOS gives us the boot drive in DL
    mov [boot_drive], dl

    ; -------------------------
    ; Print boot message
    ; -------------------------
    mov si, message
    call print_string

    ; -------------------------
    ; Destination for Stage 2
    ; ES:BX = 0000:8000
    ; -------------------------
    xor ax, ax
    mov es, ax
    mov bx, 0x8000

    ; -------------------------
    ; BIOS INT 13h
    ; Read Sector 2
    ; -------------------------
    mov ah, 0x02
    mov al, 0x01        ; 1 sector

    mov ch, 0x00        ; Cylinder 0
    mov cl, 0x02        ; Sector 2
    mov dh, 0x00        ; Head 0

    mov dl, [boot_drive]

    int 0x13

    jc disk_error

    ; -------------------------
    ; Stage 2 loaded
    ; -------------------------
    mov si, success
    call print_string

    ; -------------------------
    ; Transfer execution
    ; -------------------------
    jmp 0x0000:0x8000


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


; ==================================
; Disk error
; ==================================

disk_error:
    mov si, error
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; Data
; ==================================

message db "MyOS booting...", 13, 10, 0

success db "Stage 2 loaded. Jumping...", 13, 10, 0

error db "Disk read failed!", 13, 10, 0

boot_drive db 0


; ==================================
; Boot signature
; ==================================

times 510 - ($ - $$) db 0

dw 0xaa55