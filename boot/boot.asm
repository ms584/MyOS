bits 16
org 0x7c00

STAGE2_LOAD_ADDR equ 0x8000
STAGE2_SECTORS   equ 4

start:
    ; -------------------------
    ; Setup segments + stack
    ; -------------------------
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    cld

    ; BIOS tells us which drive booted us
    mov [boot_drive], dl

    ; -------------------------
    ; Print boot message
    ; -------------------------
    mov si, boot_message
    call print_string

    ; -------------------------
    ; Destination
    ; ES:BX = 0000:8000
    ; -------------------------
    xor ax, ax
    mov es, ax
    mov bx, STAGE2_LOAD_ADDR

    ; -------------------------
    ; Read Stage 2
    ;
    ; AH = 02h : BIOS read
    ; AL = number of sectors
    ; CH = cylinder
    ; CL = starting sector
    ; DH = head
    ; DL = drive
    ; -------------------------
    mov ah, 0x02
    mov al, STAGE2_SECTORS

    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [boot_drive]

    int 0x13

    jc disk_error

    ; -------------------------
    ; Read succeeded
    ; -------------------------
    mov si, success_message
    call print_string

    ; -------------------------
    ; Execute Stage 2
    ; -------------------------
    jmp 0x0000:STAGE2_LOAD_ADDR


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


; ==================================
; Disk error
; ==================================

disk_error:
    mov si, error_message
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; Data
; ==================================

boot_message:
    db "MyOS booting...", 13, 10, 0

success_message:
    db "Loaded 4 Stage 2 sectors.", 13, 10, 0

error_message:
    db "Disk read failed!", 13, 10, 0

boot_drive:
    db 0


; ==================================
; Boot sector
; ==================================

times 510 - ($ - $$) db 0
dw 0xaa55