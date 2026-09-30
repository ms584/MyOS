#!/usr/bin/env bash
set -euo pipefail
if [[ "${MYOS_TRACE:-0}" == 1 ]]; then
    PS4='+ ${BASH_SOURCE}:${LINENO}: '
    set -x
fi
mkdir -p build
gcc -m64 -std=c11 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic \
    -mno-red-zone -mgeneral-regs-only -fno-stack-protector -fno-asynchronous-unwind-tables \
    -fno-unwind-tables -Wall -Wextra -Werror -Wno-misleading-indentation -c user/platform-runtime.c -o build/platform-runtime.o
gcc -m64 -std=c11 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic \
    -mno-red-zone -msse2 -fno-stack-protector -fno-asynchronous-unwind-tables \
    -fno-unwind-tables -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude \
    -c user/platform-window.c -o build/platform-window.o
gcc -m64 -std=c11 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic \
    -mno-red-zone -msse2 -fno-stack-protector -fno-asynchronous-unwind-tables \
    -fno-unwind-tables -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude \
    -c user/platform-event.c -o build/platform-event.o
ld -m elf_x86_64 --build-id=none -T user/platform-runtime.ld -o build/platform-runtime.elf build/platform-runtime.o build/platform-window.o build/platform-event.o
objcopy -O binary build/platform-runtime.elf build/platform-runtime.bin
python3 tools/make_runtime_exports.py build/platform-runtime.elf build/platform-runtime-exports.h
flags=(-m64 -std=c11 -Os -ffreestanding -fno-builtin -fno-pie -fno-pic
       -mno-red-zone -mgeneral-regs-only -fno-stack-protector
       -fno-asynchronous-unwind-tables -fno-unwind-tables
       -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude -Ibuild -Ivendor/mbedtls/include
       -DTEST_CASE="${TEST_CASE:-0}" -DPROCESS_TESTS="${PROCESS_TESTS:-0}")
for app in hello port loop capacity; do
    gcc -m64 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic -mno-red-zone \
        -mgeneral-regs-only -fno-stack-protector -fno-asynchronous-unwind-tables \
        -fno-unwind-tables -Wall -Wextra -Werror -c "user/$app.c" -o "build/$app.o"
    ld -m elf_x86_64 --build-id=none -T user/app.ld -o "build/$app.elf" "build/$app.o"
    objcopy -O binary "build/$app.elf" "build/$app.bin"
    python3 tools/pack_app.py "build/$app.bin" "build/$app.app"
done
gcc -m64 -std=c11 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic \
    -mno-red-zone -mgeneral-regs-only -fno-stack-protector -fno-asynchronous-unwind-tables \
    -fno-unwind-tables -DNDEBUG -Ivendor/libwapcaplet/include -c vendor/libwapcaplet/src/libwapcaplet.c -o build/wapcaplet-lib.o
for source in user/app-runtime.c user/wapcaplet.c; do
    gcc -m64 -std=c11 -Os -mcmodel=large -ffreestanding -fno-builtin -fno-pie -fno-pic \
        -mno-red-zone -mgeneral-regs-only -fno-stack-protector -fno-asynchronous-unwind-tables \
        -fno-unwind-tables -DNDEBUG -Wall -Wextra -Werror -Iuser -Ivendor/libwapcaplet/include \
        -c "$source" -o "build/$(basename "$source" .c).o"
done
ld -m elf_x86_64 --build-id=none -T user/app.ld -o build/wapcaplet.elf \
    build/wapcaplet.o build/app-runtime.o build/wapcaplet-lib.o
objcopy -O binary build/wapcaplet.elf build/wapcaplet.bin
python3 tools/pack_app.py build/wapcaplet.bin build/wapcaplet.app
objects=()
for source in kernel/kernel.c kernel/boot.c kernel/app.c kernel/runtime.c kernel/native-ui.c drivers/console.c drivers/keyboard.c memory/memory.c cpu/interrupts.c cpu/scheduler.c cpu/gdt.c memory/process.c user/process-test.c kernel/syscall.c user/syscall-test.c fs/ramfs.c fs/asset.c kernel/elf.c kernel/elf-link.c fs/fs-test.c user/fs-test.c shell/shell.c drivers/ata.c drivers/storage.c drivers/rtl8139.c net/net.c net/tcp.c drivers/sb16.c audio/audio.c security/security.c security/tls_port.c security/tls.c drivers/graphics.c drivers/mouse.c gui/desktop.c gui/widgets.c gui/settings.c gui/calculator.c gui/snake.c gui/browser.c video/video.c; do
    obj="build/${source//\//-}"
    obj="${obj%.c}.o"
    gcc "${flags[@]}" -c "$source" -o "$obj"
    objects+=("$obj")
done
# Vendored Mbed TLS 3.6.7, built without host libc or system TLS.
tls_objects=()
for source in vendor/mbedtls/library/*.c; do
    obj="build/mbedtls-$(basename "${source%.c}").o"
    gcc -m64 -std=c11 -Os -ffreestanding -fno-builtin -fno-pie -fno-pic \
        -mno-red-zone -mgeneral-regs-only -fno-stack-protector \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        -ffunction-sections -fdata-sections -U_FORTIFY_SOURCE -Ivendor/mbedtls/include \
        -Ivendor/mbedtls/library -c "$source" -o "$obj"
    tls_objects+=("$obj")
done
ar rcs build/libmbedtls-myos.a "${tls_objects[@]}"
nasm -f elf64 user/app-blob.asm -o build/app-blob.o
nasm -f elf64 user/platform-runtime-blob.asm -o build/platform-runtime-blob.o
nasm -f elf64 user/elf-start.asm -o build/elf-start.o
nasm -f elf64 kernel/entry.asm -o build/entry.o
nasm -f elf64 cpu/interrupts.asm -o build/interrupt-stubs.o
nasm -f elf64 cpu/context-probe.asm -o build/context-probe.o
nasm -f bin user/demo.asm -o build/user-demo.bin
nasm -f elf64 user/blob.asm -o build/user-blob.o
ld -m elf_x86_64 --gc-sections --build-id=none -Map=build/kernel.map -T kernel/linker.ld -o build/kernel.elf build/entry.o "${objects[@]}" build/interrupt-stubs.o build/context-probe.o build/user-blob.o build/app-blob.o build/platform-runtime-blob.o build/elf-start.o build/libmbedtls-myos.a "$(gcc -print-libgcc-file-name)"
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
