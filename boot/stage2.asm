bits 16
org 0x8000

KERNEL_LOAD_SEGMENT equ 0x1000
KERNEL_SECTORS      equ 4

stage2_start:
    xor ax, ax
    mov ds, ax

    cld

    ; Save boot drive passed from Stage 1
    mov [boot_drive], dl

    ; -------------------------
    ; Stage 2 message
    ; -------------------------
    mov si, stage2_message
    call print_string

    ; -------------------------
    ; Kernel destination
    ;
    ; ES:BX = 1000:0000
    ;
    ; Physical address:
    ; 0x1000 * 16 = 0x10000
    ; -------------------------

    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax

    xor bx, bx

    ; -------------------------
    ; BIOS disk read
    ;
    ; Kernel starts at
    ; BIOS Sector 6
    ; -------------------------

    mov ah, 0x02
    mov al, KERNEL_SECTORS

    mov ch, 0x00
    mov cl, 0x06
    mov dh, 0x00

    mov dl, [boot_drive]

    int 0x13

    jc disk_error

    ; -------------------------
    ; Kernel loaded
    ; -------------------------

    mov si, kernel_loaded_message
    call print_string

    ; -------------------------
    ; Jump to Kernel
    ;
    ; 1000:0000 = 0x10000
    ; -------------------------

    jmp KERNEL_LOAD_SEGMENT:0x0000


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
    mov si, disk_error_message
    call print_string

.hang:
    cli
    hlt
    jmp .hang


; ==================================
; Data
; ==================================

stage2_message:
    db "Stage 2 running...", 13, 10, 0

kernel_loaded_message:
    db "Kernel loaded. Jumping...", 13, 10, 0

disk_error_message:
    db "Kernel disk read failed!", 13, 10, 0

boot_drive:
    db 0


; ==================================
; Stage 2 = 4 sectors
; ==================================

times 2048 - ($ - $$) db 0