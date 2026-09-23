"""Run VGA behavior checks on the real driver in isolated QEMU images.

Run from the repository root: python3 test-vga.py
Requires the same tools as build-phase22.sh, plus Python 3 and QEMU.
The normal kernel source and boot image are never replaced.
"""
import pathlib
import subprocess

root = pathlib.Path.cwd()
build = root / 'build/vga-tests'
build.mkdir(parents=True, exist_ok=True)
subprocess.run(['bash', 'build-phase22.sh'], check=True)

prefix = r'''
#include "vga.h"
static unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static void finish(unsigned char code) {
    outb(0xf4, code);
    for (;;) __asm__ volatile ("cli; hlt");
}
static void expect(int condition) {
    if (!condition) finish(0x11);
}
static unsigned int hardware_cursor(void) {
    outb(0x3d4, 0x0e);
    unsigned int high = inb(0x3d5);
    outb(0x3d4, 0x0f);
    return (high << 8) | inb(0x3d5);
}
static volatile unsigned char bss_probe[257];
void kernel_main(void) {
    volatile unsigned short *screen = (volatile unsigned short *)0xb8000;
    vga_clear();
'''
cases = {
    'bss-zero': r'''
        for (unsigned int i = 0; i < sizeof(bss_probe); ++i)
            expect(bss_probe[i] == 0);
        expect(hardware_cursor() == 0);
        expect(screen[0] == 0x0f20);
    ''',
    'clear': r'''
        kprint("old text");
        vga_clear();
        for (unsigned int i = 0; i < 2000; ++i) expect(screen[i] == 0x0f20);
        expect(hardware_cursor() == 0);
    ''',
    'cursor': r'''
        vga_set_cursor(10, 5);
        expect(hardware_cursor() == 410);
        kprint("X");
        for (unsigned int i = 0; i < 2000; ++i)
            expect(screen[i] == (i == 410 ? 0x0f58 : 0x0f20));
        expect(hardware_cursor() == 411);
    ''',
    'wrap': r'''
        for (unsigned int i = 0; i < 81; ++i) vga_putchar('A');
        for (unsigned int i = 0; i < 2000; ++i)
            expect(screen[i] == (i < 81 ? 0x0f41 : 0x0f20));
        expect(hardware_cursor() == 81);
    ''',
    'scroll': r'''
        kprint("TOP\n");
        for (unsigned int i = 0; i < 24; ++i) kprint("line\n");
        const char *word = "line";
        for (unsigned int i = 0; i < 2000; ++i) {
            unsigned char ch = (i < 1920 && i % 80 < 4) ? word[i % 80] : ' ';
            expect(screen[i] == (0x0f00u | ch));
        }
        expect(hardware_cursor() == 1920);
    ''',
    'carriage-return': r'''
        kprint("abc\rX");
        expect(screen[0] == 0x0f58 && screen[1] == 0x0f62 && screen[2] == 0x0f63);
        expect(hardware_cursor() == 1);
    ''',
    'bottom-right-and-bounds': r'''
        vga_set_cursor(79, 24);
        vga_putchar('Z');
        for (unsigned int i = 0; i < 2000; ++i)
            expect(screen[i] == (i == 1919 ? 0x0f5a : 0x0f20));
        expect(hardware_cursor() == 1920);
        vga_set_cursor(80, 25);
        expect(hardware_cursor() == 1920);
        vga_putchar('Q');
        expect(screen[1920] == 0x0f51);
        expect(hardware_cursor() == 1921);
    ''',
}

entry_source = (root / 'kernel/entry.asm').read_text()
entry_source = entry_source.replace('    xor eax, eax',
    '    mov al, 0xa5\n    mov edi, __bss_start\n'
    '    mov ecx, __bss_end\n    sub ecx, edi\n    rep stosb\n    xor eax, eax', 1)
(build / 'entry.asm').write_text(entry_source)
subprocess.run(['nasm', '-f', 'elf32', str(build / 'entry.asm'),
                '-o', str(build / 'entry.o')], check=True)

for name, body in cases.items():
    source = build / (name + '.c')
    obj = build / (name + '.o')
    elf = build / (name + '.elf')
    binary = build / (name + '.bin')
    image = build / (name + '.img')
    source.write_text(prefix + body + '\nfinish(0x10);\n}\n')
    subprocess.run(['gcc', '-m32', '-std=c11', '-Os', '-ffreestanding',
                    '-fno-pie', '-fno-pic', '-fno-stack-protector',
                    '-fno-asynchronous-unwind-tables', '-fno-unwind-tables',
                    '-Wall', '-Wextra', '-Werror', '-Iinclude',
                    '-c', str(source), '-o', str(obj)], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '--build-id=none', '-T',
                    'kernel/linker.ld', '-o', str(elf), str(build / 'entry.o'),
                    str(obj), 'build/vga.o'], check=True)
    subprocess.run(['objcopy', '-O', 'binary', str(elf), str(binary)], check=True)
    kernel = binary.read_bytes()
    assert len(kernel) <= 4096, 'Test kernel exceeds loader capacity'
    disk = bytearray((root / 'build/myos-current.img').read_bytes())
    disk[2560:6656] = kernel.ljust(4096, b'\0')
    image.write_bytes(disk)
    result = subprocess.run([
        'qemu-system-x86_64', '-display', 'none', '-monitor', 'none',
        '-serial', 'none', '-no-reboot', '-device',
        'isa-debug-exit,iobase=0xf4,iosize=0x04', '-drive',
        f'format=raw,file={image},if=floppy', '-boot', 'order=a'], timeout=15)
    assert result.returncode == 33, f'{name}: QEMU exit {result.returncode}, expected 33'
    print(f'PASS: {name}', flush=True)

subprocess.run(['python3', 'test-boot.py'], check=True)
print('PASS: all seven VGA/startup cases and the unchanged normal startup image')
