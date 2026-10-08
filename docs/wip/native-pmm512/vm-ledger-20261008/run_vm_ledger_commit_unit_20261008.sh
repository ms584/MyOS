#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
unit=$(mktemp -d build/vm-ledger-commit-unit-XXXXXX)
cp -a include "$unit/"
mkdir "$unit/memory" "$unit/build"
cp "$stage/process-vm-ledger-clean-20261008.c" "$unit/memory/process.c"
cp "$stage/vm-ledger-20261008.h" "$unit/include/vm-ledger.h"
cp build/*.h "$unit/build/"
cd "$unit"
gcc -std=c11 -O1 -g -ffreestanding -fno-builtin -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections -DPROCESS_VM_UNIT -DPROCESS_RUNTIME_EXTERNAL=0 -Iinclude -Ibuild -c memory/process.c -o build/process.o
gcc -std=c11 -O1 -g -ffreestanding -fno-builtin -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections -Iinclude "$stage/vm-commit-unit.c" build/process.o -Wl,--gc-sections -o build/commit-unit
ASAN_OPTIONS=detect_leaks=1 build/commit-unit
gcc -std=c11 -O1 -g -ffreestanding -fno-builtin -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections -Iinclude "$stage/vm-file-unit.c" "$stage/vm-file-immutable-stubs-20261008.c" build/process.o -Wl,--gc-sections -o build/file-unit
ASAN_OPTIONS=detect_leaks=1 build/file-unit
printf 'EVIDENCE %s\n' "$PWD"
