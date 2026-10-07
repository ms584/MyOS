#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
probe=$(mktemp -d /home/xytron404/record-lock-unit-XXXXXX)
cp "$stage/record-lock.c" "$stage/record-lock.h" "$stage/record-lock-unit.c" "$stage/fs-unit-platform.h" "$probe/"
cp /home/xytron404/myos/include/os.h "$probe/"
cd "$probe"
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -include fs-unit-platform.h record-lock.c record-lock-unit.c -o test
ASAN_OPTIONS=detect_leaks=1 ./test
printf 'RECORD_LOCK_UNIT=%s\n' "$probe"
