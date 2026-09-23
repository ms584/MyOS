"""Boot isolated fault images and verify the real exception screen via QMP."""
from pathlib import Path
import subprocess

subprocess.run(['bash', 'build.sh'], check=True)
b = Path('build/exception-tests')
b.mkdir(exist_ok=True)
for name, code, title, vector, error in [
    ('ud2', '__asm__ volatile (".global fault_site\\nfault_site: ud2");',
     'Invalid Opcode', 6, 0),
    ('gp', '__asm__ volatile ("mov $0x18, %%ax\\n.global fault_site\\nfault_site: mov %%ax, %%ds" : : : "eax");',
     'General Protection Fault', 13, 0x18),
]:
    source = b / (name + '.c')
    source.write_text('#include "vga.h"\n#include "idt.h"\nvoid kernel_main(void) {'
                      'vga_clear(); idt_init();' + code + 'for (;;) __asm__ volatile("cli; hlt");}\n')
    obj, elf, binary, image = [b / (name + suffix) for suffix in ('.o', '.elf', '.bin', '.img')]
    subprocess.run(['gcc', '-m32', '-Os', '-ffreestanding', '-fno-pie', '-fno-pic',
                    '-fno-stack-protector', '-fno-asynchronous-unwind-tables',
                    '-Wall', '-Wextra', '-Werror', '-Iinclude', '-c', str(source),
                    '-o', str(obj)], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '--build-id=none', '-T', 'kernel/linker.ld',
                    '-o', str(elf), 'build/entry.o', str(obj), 'build/vga.o',
                    'build/idt.o', 'build/isr.o', 'build/timer.o', 'build/irq.o'], check=True)
    subprocess.run(['objcopy', '-O', 'binary', str(elf), str(binary)], check=True)
    symbols = subprocess.check_output(['nm', str(elf)], text=True)
    fault = next(int(line.split()[0], 16) for line in symbols.splitlines()
                 if line.split()[-1] == 'fault_site')
    kernel = binary.read_bytes()
    assert len(kernel) <= 4096
    disk = bytearray(Path('build/myos-current.img').read_bytes())
    disk[2560:6656] = kernel.ljust(4096, b'\0')
    image.write_bytes(disk)
    expected = [f'CPU Exception: {title}', f'Vector: 0x{vector:08X}',
                f'Error: 0x{error:08X}', f'EIP: 0x{fault:08X}', 'CPU halted']
    # Reuse the startup test's QMP transport with fault-specific assertions.
    check = Path('test-boot.py').read_text()
    check = check.replace('build/myos-current.img', str(image))
    start = check.index('    expected = ')
    end = check.index('\n', start)
    check = check[:start] + '    expected = ' + repr(expected) + check[end:]
    check = check.replace('lines[:4]', 'lines[:5]').replace('lines[4:]', 'lines[5:]')
    check = check.replace('all startup lines', name + ' vector, error code and exact fault EIP')
    subprocess.run(['python3', '-c', check], check=True)
subprocess.run(['python3', 'test-boot.py'], check=True)
print('PASS: exceptions with and without CPU error codes; normal image unchanged')
