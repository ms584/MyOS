#!/usr/bin/env bash
# Isolated code-generation measurement, NOT a native validation or production change.
set -euo pipefail
out=build/runtime-pic-evaluation
mkdir -p "$out"
common=(-m64 -std=c11 -Os -mcmodel=small -fPIC -fno-semantic-interposition
        -ffreestanding -fno-builtin -mno-red-zone -fno-stack-protector
        -fno-asynchronous-unwind-tables -fno-unwind-tables
        -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude)
gcc "${common[@]}" -mgeneral-regs-only -c user/platform-runtime.c -o "$out/runtime.o"
gcc "${common[@]}" -msse2 -c user/platform-window.c -o "$out/window.o"
gcc "${common[@]}" -msse2 -c user/platform-event.c -o "$out/event.o"
ld -m elf_x86_64 --build-id=none -T tools/runtime-pic-probe.ld -o "$out/runtime.elf" \
   "$out/runtime.o" "$out/window.o" "$out/event.o"
objcopy -O binary "$out/runtime.elf" "$out/runtime.bin"
python3 tools/make_runtime_exports.py "$out/runtime.elf" "$out/exports.h"
stat -c '%n %s bytes' build/platform-runtime.bin "$out/runtime.bin"
readelf -rW "$out/runtime.elf"
readelf -SW "$out/runtime.elf"
readelf -lW "$out/runtime.elf"
