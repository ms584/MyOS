#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
source=/tmp/myos-file-map-u9NQ2N
unit=$(mktemp -d /tmp/myos-file-map-unit-XXXXXX)
cp -a "$source/include" "$unit/"
mkdir "$unit/memory" "$unit/build"
cp "$stage/process-file-map.c" "$unit/memory/process.c"
cp "$source/build/"*.h "$unit/build/"
cd "$unit"
printf 'PRIVATE_FILE_MAP_UNIT=%s\n' "$unit"
gcc -std=c11 -O1 -g -ffreestanding -fno-builtin -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections -DPROCESS_VM_UNIT -DPROCESS_RUNTIME_EXTERNAL=0 -Iinclude -Ibuild -c memory/process.c -o build/process.o
gcc -std=c11 -O1 -g -ffreestanding -fno-builtin -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections -fdata-sections -Iinclude "$stage/vm-file-unit.c" build/process.o -Wl,--gc-sections -o build/unit
ASAN_OPTIONS=detect_leaks=1 build/unit
