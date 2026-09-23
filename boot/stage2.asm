bits 16
org 0x8000

KERNEL_SEGMENT equ 0x1000
KERNEL_SECTORS equ 8

stage2_start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    ; Establish 80x25 color text mode before protected mode.
    mov ax, 0x0003
    int 0x10

    ; -------------------------
    ; Load kernel
    ; BIOS sectors 6-13
    ; -------------------------

    mov ax, KERNEL_SEGMENT
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, KERNEL_SECTORS

    mov ch, 0
    mov cl, 6
    mov dh, 0
    mov dl, [boot_drive]

    int 0x13
    jc disk_error

    ; -------------------------
    ; Disable interrupts
    ; -------------------------

    cli

    ; -------------------------
    ; Enable A20
    ; Fast A20 gate
    ; -------------------------

    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; -------------------------
    ; Load GDT
    ; -------------------------

    lgdt [gdt_descriptor]

    ; -------------------------
    ; Enable Protected Mode
    ; -------------------------

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump reloads CS
    jmp CODE_SEG:protected_mode


disk_error:
    mov ah, 0x0e
    mov al, 'E'
    mov bh, 0x00
    int 0x10

.hang:
    cli
    hlt
    jmp .hang


; ============================
; GDT
; ============================

gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


; ============================
; Protected Mode
; ============================

bits 32

protected_mode:
    mov ax, DATA_SEG

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    ; kernel linked at 0x10000
    jmp 0x10000


boot_drive:
    db 0


; Stage 2 remains 4 sectors
times 2048 - ($ - $$) db 0
