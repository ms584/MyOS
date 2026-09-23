#!/usr/bin/env bash
set -euo pipefail

# Run from the existing MyOS repository root.
for file in boot/boot.asm boot/stage2.asm kernel/entry.asm kernel/linker.ld; do
    test -f "$file" || { echo "Missing existing repo file: $file" >&2; exit 1; }
done
mkdir -p build
nasm -f bin boot/boot.asm -o build/boot.bin
nasm -f bin boot/stage2.asm -o build/stage2.bin
nasm -f elf32 kernel/entry.asm -o build/entry.o
flags=(-m32 -std=c11 -Os -ffreestanding -fno-pie -fno-pic
       -fno-stack-protector -fno-asynchronous-unwind-tables
       -fno-unwind-tables -Wall -Wextra -Werror -Iinclude)
gcc "${flags[@]}" -c kernel/kernel.c -o build/kernel.o
gcc "${flags[@]}" -c drivers/vga.c -o build/vga.o
gcc "${flags[@]}" -c cpu/idt.c -o build/idt.o
nasm -f elf32 cpu/isr.asm -o build/isr.o
ld -m elf_i386 --build-id=none -T kernel/linker.ld \
   -o build/kernel.elf build/entry.o build/kernel.o build/vga.o build/idt.o build/isr.o
objcopy -O binary build/kernel.elf build/kernel.bin

test "$(stat -c %s build/boot.bin)" -eq 512
test "$(stat -c %s build/stage2.bin)" -eq 2048
# Existing Stage 2 reads eight sectors beginning at BIOS sector 6.
test "$(stat -c %s build/kernel.bin)" -le 4096 || {
    echo 'Kernel exceeds the existing 8-sector loader capacity.' >&2; exit 1;
}
test "$(nm -n build/kernel.elf | awk '$3 == "kernel_entry" {print $1}')" = 00010000 || {
    echo 'kernel_entry must be at 0x10000 for the existing raw jump.' >&2; exit 1;
}
# Write a separate image, preserving the previous working image.
dd if=/dev/zero of=build/myos-phase23.img bs=512 count=2880 status=none
dd if=build/boot.bin of=build/myos-phase23.img bs=512 seek=0 conv=notrunc status=none
dd if=build/stage2.bin of=build/myos-phase23.img bs=512 seek=1 conv=notrunc status=none
dd if=build/kernel.bin of=build/myos-phase23.img bs=512 seek=5 conv=notrunc status=none
stat -c '%n: %s bytes' build/kernel.bin build/myos-phase23.img
