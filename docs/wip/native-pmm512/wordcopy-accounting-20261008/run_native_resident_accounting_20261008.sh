#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
grep -q '^TERMINAL_EXIT=' cobalt-wordcopy-memory-check.status
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing live native tree mutation'; exit 1
fi
cp "$stage/fs-resident-accounting-20261008.h" include/fs.h
cp "$stage/ramfs-resident-accounting-20261008.c" fs/ramfs.c
cp "$stage/elf-resident-accounting-20261008.c" kernel/elf.c
cp "$stage/sb16-queue-heap-20261008.c" drivers/sb16.c
printf 'RUNNING\n' >native-resident-accounting.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >native-resident-accounting.status' EXIT
bash build.sh >native-resident-accounting-build.log 2>&1
python3 tools/build_uefi.py >native-resident-accounting-uefi-build.log 2>&1
python3 "$stage/test_named_shm_lifetime_20261008.py" >native-resident-accounting-lifetime-bios.log 2>&1
python3 "$stage/test_named_shm_lifetime_20261008.py" --uefi >native-resident-accounting-lifetime-uefi.log 2>&1
python3 tests/test_shared_memory.py >native-resident-accounting-shared-bios.log 2>&1
python3 tests/test_shared_memory.py --uefi >native-resident-accounting-shared-uefi.log 2>&1
