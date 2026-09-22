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

    ; -------------------------
    ; Save boot drive
    ; BIOS gives boot drive in DL
    ; -------------------------
    mov [boot_drive], dl

    ; -------------------------
    ; Print boot message
    ; -------------------------
    mov si, message
    call print_string

    ; -------------------------
    ; Prepare destination
    ; ES:BX = 0000:8000
    ; -------------------------
    xor ax, ax
    mov es, ax
    mov bx, 0x8000

    ; -------------------------
    ; Read Sector 2
    ; -------------------------
    mov ah, 0x02        ; BIOS read sectors
    mov al, 0x01        ; Read 1 sector
    mov ch, 0x00        ; Cylinder 0
    mov cl, 0x02        ; Sector 2
    mov dh, 0x00        ; Head 0
    mov dl, [boot_drive]

    int 0x13

    ; Check Carry Flag
    jc disk_error

    ; -------------------------
    ; Disk read successful
    ; -------------------------
    mov si, success
    call print_string

    ; -------------------------
    ; Print data loaded at
    ; 0000:8000
    ; -------------------------
    mov si, 0x8000
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; print_string
; DS:SI -> null-terminated string
; ==================================
print_string:
.next:
    lodsb

    test al, al
    jz .done

    mov ah, 0x0e
    mov bh, 0
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

    cli
    hlt
    jmp $


; ==================================
; Data
; ==================================

message db "MyOS booting...", 13, 10, 0

success db "Disk read successful!", 13, 10, 0

error db "Disk read failed!", 13, 10, 0

boot_drive db 0


; ==================================
; Boot sector padding
; ==================================

times 510 - ($ - $$) db 0

dw 0xaa55