#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
probe=$(mktemp -d /home/xytron404/shm-unit-XXXXXX)
cp "$stage/shm-backing.c" "$stage/shm-backing.h" "$stage/shm-unit-platform.h" "$stage/fs-unit-platform.h" "$stage/shm-backing-unit.c" "$probe/"
cp /home/xytron404/myos/include/os.h "$probe/"
cd "$probe"
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -include shm-unit-platform.h shm-backing.c shm-backing-unit.c -o test
ASAN_OPTIONS=detect_leaks=1 ./test
printf 'SHM_BACKING_UNIT=%s\n' "$probe"
