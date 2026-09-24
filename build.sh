#!/usr/bin/env bash
set -euo pipefail
if [[ "${MYOS_TRACE:-0}" == 1 ]]; then
    PS4='+ ${BASH_SOURCE}:${LINENO}: '
    set -x
fi
mkdir -p build
flags=(-m64 -std=c11 -Os -ffreestanding -fno-builtin -fno-pie -fno-pic
       -mno-red-zone -mgeneral-regs-only -fno-stack-protector
       -fno-asynchronous-unwind-tables -fno-unwind-tables
       -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude
       -DTEST_CASE="${TEST_CASE:-0}" -DPROCESS_TESTS="${PROCESS_TESTS:-0}")
objects=()
for source in kernel/kernel.c kernel/runtime.c drivers/console.c drivers/keyboard.c memory/memory.c cpu/interrupts.c cpu/scheduler.c cpu/gdt.c memory/process.c user/process-test.c kernel/syscall.c user/syscall-test.c fs/ramfs.c fs/fs-test.c user/fs-test.c shell/shell.c drivers/ata.c drivers/storage.c drivers/graphics.c; do
    obj="build/${source//\//-}"
    obj="${obj%.c}.o"
    gcc "${flags[@]}" -c "$source" -o "$obj"
    objects+=("$obj")
done
nasm -f elf64 kernel/entry.asm -o build/entry.o
nasm -f elf64 cpu/interrupts.asm -o build/interrupt-stubs.o
nasm -f elf64 cpu/context-probe.asm -o build/context-probe.o
nasm -f bin user/demo.asm -o build/user-demo.bin
nasm -f elf64 user/blob.asm -o build/user-blob.o
ld -m elf_x86_64 --build-id=none -T kernel/linker.ld -o build/kernel.elf build/entry.o "${objects[@]}" build/interrupt-stubs.o build/context-probe.o build/user-blob.o
objcopy -O binary build/kernel.elf build/kernel.bin
size=$(stat -c %s build/kernel.bin)
test "$size" -le 262144 || { echo 'Kernel exceeds 256 KiB loader limit' >&2; exit 1; }
sectors=$(((size + 511) / 512))
nasm -f bin boot/boot.asm -o build/boot.bin
video=()
if [[ "${TEXT_ONLY:-0}" == 1 ]]; then video=(-DTEXT_ONLY=1); fi
nasm -f bin -DKERNEL_SECTORS="$sectors" "${video[@]}" boot/stage2.asm -o build/stage2.bin
test "$(stat -c %s build/boot.bin)" -eq 512
test "$(stat -c %s build/stage2.bin)" -eq 8192
dd if=/dev/zero of=build/myos-current.img bs=512 count=2880 status=none
dd if=build/boot.bin of=build/myos-current.img bs=512 seek=0 conv=notrunc status=none
dd if=build/stage2.bin of=build/myos-current.img bs=512 seek=1 conv=notrunc status=none
dd if=build/kernel.bin of=build/myos-current.img bs=512 seek=17 conv=notrunc status=none
echo "Kernel: $size bytes ($sectors sectors); image: build/myos-current.img"
