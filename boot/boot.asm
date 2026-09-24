bits 16
org 0x7c00
    jmp 0:start
start:
    cli
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00
    sti
    cld
    mov [drive],dl
%ifdef BIOS_HDD
    ; EDD packet reads use LBA, independent of BIOS CHS translation.
    mov ah,0x41
    mov bx,0x55aa
    mov dl,[drive]
    int 0x13
    jc .error
    cmp bx,0xaa55
    jne .error
    test cx,1
    jz .error
%endif
    mov byte [tries],3
.retry:
%ifdef BIOS_HDD
    mov si,dap
    mov ah,0x42
    mov dl,[drive]
%else
    xor ax,ax
    mov es,ax
    mov bx,0x8000
    mov ax,0x0210
    mov cx,2
    xor dh,dh
    mov dl,[drive]
%endif
    int 0x13
    jnc .loaded
    xor ax,ax
    mov dl,[drive]
    int 0x13
    dec byte [tries]
    jnz .retry
.error:
    mov ax,0x0e45
    int 0x10
.halt:
    cli
    hlt
    jmp .halt
.loaded:
    mov dl,[drive]
    jmp 0:0x8000
drive: db 0
tries: db 0
%ifdef BIOS_HDD
align 4
dap: db 16,0
     dw 16,0x8000,0
     dq 1
%endif
times 510-($-$$) db 0
dw 0xaa55
