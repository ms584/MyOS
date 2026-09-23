"""Verify 100 real PIT IRQs, register/DF restoration, PIC masks and EOI."""
from pathlib import Path
import subprocess

subprocess.run(['bash', 'build.sh'], check=True)
b = Path('build/timer-tests')
b.mkdir(exist_ok=True)
(b/'kernel.c').write_text(r'''
#include "idt.h"
#include "timer.h"
#include "io.h"
extern unsigned int probe_irq(void);
void kernel_main(void) {
    idt_init(); timer_init();
    int ok = inb(0x21) == 0xfe && inb(0xa1) == 0xff;
    for (unsigned int i = 0; i < 100; ++i) {
        unsigned int before = timer_ticks();
        ok &= probe_irq() == 1;
        ok &= timer_ticks() > before;
    }
    outb(0x20, 0x0b);
    ok &= inb(0x20) == 0; /* EOI cleared the master in-service register. */
    ok &= timer_ticks() >= 100;
    outb(0xf4, ok ? 0x10 : 0x11);
    for (;;) __asm__ volatile("cli; hlt");
}
''')
asm='''bits 32
section .text
global probe_irq
probe_irq:
    push ebx
    push esi
    push edi
    push ebp
'''
registers = ['eax','ebx','ecx','edx','esi','edi','ebp']
for i, reg in enumerate(registers):
    asm += f'    mov {reg}, {0x12340000+i}\n'
asm += '    std\n    sti\n    hlt\n    cli\n'
for i, reg in enumerate(registers):
    asm += f'    cmp {reg}, {0x12340000+i}\n    jne .fail\n'
asm += '''    pushfd
    pop eax
    test eax, 0x400
    jz .fail
    mov eax, 1
    jmp .done
.fail:
    xor eax, eax
.done:
    cld
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
'''
(b/'probe.asm').write_text(asm)
subprocess.run(['nasm','-f','elf32',str(b/'probe.asm'),'-o',str(b/'probe.o')], check=True)
subprocess.run(['gcc','-m32','-Os','-ffreestanding','-fno-pie','-fno-pic',
                '-fno-stack-protector','-fno-asynchronous-unwind-tables',
                '-Wall','-Wextra','-Werror','-Iinclude','-c',str(b/'kernel.c'),
                '-o',str(b/'kernel.o')], check=True)
subprocess.run(['ld','-m','elf_i386','--build-id=none','-T','kernel/linker.ld',
                '-o',str(b/'kernel.elf'),'build/entry.o',str(b/'kernel.o'),
                str(b/'probe.o'),'build/vga.o','build/idt.o','build/isr.o',
                'build/timer.o','build/irq.o'], check=True)
subprocess.run(['objcopy','-O','binary',str(b/'kernel.elf'),str(b/'kernel.bin')],check=True)
kernel=(b/'kernel.bin').read_bytes()
assert len(kernel)<=4096
disk=bytearray(Path('build/myos-current.img').read_bytes())
disk[2560:6656]=kernel.ljust(4096,b'\0')
(b/'test.img').write_bytes(disk)
result=subprocess.run(['qemu-system-x86_64','-display','none','-monitor','none',
    '-serial','none','-no-reboot','-device','isa-debug-exit,iobase=0xf4,iosize=0x04',
    '-drive',f'format=raw,file={b}/test.img,if=floppy','-boot','order=a'],timeout=15)
assert result.returncode==33, f'Timer test failed: QEMU exit {result.returncode}'
print('PASS: 100 real PIT IRQs, all general registers and DF restored, PIC masks and EOI')
