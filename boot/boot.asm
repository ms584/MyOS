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
    mov si,3
.retry:
    xor ax,ax
    mov es,ax
    mov bx,0x8000
    mov ax,0x0210
    mov cx,2
    xor dh,dh
    mov dl,[drive]
    int 0x13
    jnc .loaded
    xor ax,ax
    mov dl,[drive]
    int 0x13
    dec si
    jnz .retry
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
times 510-($-$$) db 0
dw 0xaa55
