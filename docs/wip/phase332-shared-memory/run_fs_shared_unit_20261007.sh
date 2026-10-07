#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
probe=$(mktemp -d /home/xytron404/fs-shared-unit-XXXXXX)
for source in shm-backing.c shm-backing.h shm-unit-platform.h fs-unit-platform.h shm-backing-unit.c fs-shared-unit-20261007.c fs-volume-call.h; do cp "$stage/$source" "$probe/"; done
cp "$stage/ramfs-shared-20261007.c" "$probe/ramfs.c"
cp "$stage/fs-shared-20261007.h" "$probe/fs.h"
cp "$stage/snapshot-shared-20261007.inc" "$probe/snapshot-impl.inc"
cp /home/xytron404/myos/include/{os.h,fs-snapshot.h,storage.h,asset.h} "$probe/"
cd "$probe"
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -include shm-unit-platform.h ramfs.c shm-backing.c fs-shared-unit-20261007.c -o test
ASAN_OPTIONS=detect_leaks=1 ./test
printf 'FS_SHARED_UNIT=%s\n' "$probe"
