bits 16
org 0x8000
%ifndef KERNEL_SECTORS
%error KERNEL_SECTORS required
%endif
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
    mov word [0x7000],0
    mov byte [0x7004],0
    ; Preserve the BIOS 8x16 font for the framebuffer console.
    mov ax,0x1130
    mov bh,6
    int 0x10
    push ds
    mov ax,es
    mov ds,ax
    mov si,bp
    xor ax,ax
    mov es,ax
    mov di,0xa000
    mov cx,2048
    rep movsw
    pop ds
    ; E820 memory map: count at 0x7000, 24-byte records at 0x7400.
    xor ebx,ebx
    mov di,0x7400
.e820:
    mov dword [es:di+20],1
    mov eax,0xe820
    mov edx,0x534d4150
    mov ecx,24
    int 0x15
    jc .map_done
    cmp eax,0x534d4150
    jne error
    cmp ecx,20
    jb error
    inc word [0x7000]
    add di,24
    cmp word [0x7000],64
    jae error
    test ebx,ebx
    jnz .e820
.map_done:
    cmp word [0x7000],0
    je error
    ; Read one floppy CHS sector at a time, including track/head boundaries.
    mov word [lba],17
    mov word [dest],0x1000
    mov word [remaining],KERNEL_SECTORS
.sector:
    mov byte [tries],3
.retry:
    mov ax,[lba]
    xor dx,dx
    mov bx,18
    div bx
    mov cl,dl
    inc cl
    xor dx,dx
    mov bx,2
    div bx
    mov ch,al
    mov dh,dl
    mov dl,[drive]
    mov ax,[dest]
    mov es,ax
    xor bx,bx
    mov ax,0x0201
    int 0x13
    jnc .read
    xor ax,ax
    mov dl,[drive]
    int 0x13
    dec byte [tries]
    jnz .retry
    jmp error
.read:
    add word [dest],0x20
    inc word [lba]
    dec word [remaining]
    jnz .sector
    xor ax,ax
    mov es,ax
%ifndef TEXT_ONLY
    ; Choose a VBE direct-color 800x600x32 linear framebuffer mode.
    mov di,0xb000
    mov dword [es:di],0x32454256
    mov ax,0x4f00
    int 0x10
    cmp ax,0x004f
    jne .text
    mov si,[0xb00e]
    mov ax,[0xb010]
    mov fs,ax
    mov word [mode_limit],256
.mode:
    mov cx,[fs:si]
    add si,2
    cmp cx,0xffff
    je .text
    mov [mode],cx
    push si
    push fs
    mov ax,0x4f01
    mov di,0xb200
    int 0x10
    pop fs
    pop si
    cmp ax,0x004f
    jne .nextmode
    mov ax,[0xb200]
    and ax,0x91
    cmp ax,0x91
    jne .nextmode
    cmp word [0xb212],800
    jne .nextmode
    cmp word [0xb214],600
    jne .nextmode
    cmp byte [0xb219],32
    jne .nextmode
    cmp byte [0xb21b],6
    jne .nextmode
    mov bx,[mode]
    or bx,0x4000
    mov ax,0x4f02
    int 0x10
    cmp ax,0x004f
    jne .text
    mov byte [0x7004],1
    jmp .video_done
.nextmode:
    dec word [mode_limit]
    jnz .mode
%endif
.text:
    mov ax,3
    int 0x10
.video_done:
    cli
    in al,0x92
    or al,2
    and al,0xfe
    out 0x92,al
    lgdt [gdt_ptr]
    mov eax,cr0
    or eax,1
    mov cr0,eax
    jmp 0x08:protected
error:
    mov ax,0x0e45
    int 0x10
.hang:
    cli
    hlt
    jmp .hang
align 8
gdt:
    dq 0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
    dq 0x00af9a000000ffff
gdt_ptr:
    dw $-gdt-1
    dd gdt
bits 32
protected:
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov gs,ax
    mov esp,0x90000
    ; Require long mode and NX (used by heap mappings).
    mov eax,0x80000000
    cpuid
    cmp eax,0x80000001
    jb .halt
    mov eax,0x80000001
    cpuid
    and edx,0x20100000
    cmp edx,0x20100000
    jne .halt
    ; Identity-map 4 GiB using 2 MiB pages; tables occupy 0x1000..0x6fff.
    xor eax,eax
    mov edi,0x1000
    mov ecx,0x1800
    rep stosd
    mov dword [0x1000],0x2003
    mov edi,0x2000
    mov eax,0x3003
    mov ecx,4
.pdpt:
    mov [edi],eax
    add edi,8
    add eax,0x1000
    loop .pdpt
    mov edi,0x3000
    mov eax,0x83
    mov ecx,2048
.pd:
    mov [edi],eax
    add eax,0x200000
    add edi,8
    loop .pd
    mov eax,cr4
    or eax,0x20
    mov cr4,eax
    mov eax,0x1000
    mov cr3,eax
    mov ecx,0xc0000080
    rdmsr
    or eax,0x900
    wrmsr
    mov eax,cr0
    or eax,0x80010000
    mov cr0,eax
    jmp 0x18:long_mode
.halt:
    cli
    hlt
    jmp .halt
bits 64
long_mode:
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov rsp,0x90000
    mov rax,0x10000
    jmp rax
drive: db 0
tries: db 0
lba: dw 0
dest: dw 0
remaining: dw 0
mode: dw 0
mode_limit: dw 0
times 8192-($-$$) db 0
