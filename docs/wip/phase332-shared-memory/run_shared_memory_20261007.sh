#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cd-QJYMwG
test "$(cat pmm-refs.status)" = 'TERMINAL_EXIT=0'
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >shared-memory.status' EXIT
cp "$stage/shm-backing.c" fs/
cp "$stage/shm-backing.h" include/
cp "$stage/fs-shared-20261007.h" include/fs.h
cp "$stage/ramfs-shared-20261007.c" fs/ramfs.c
cp "$stage/snapshot-shared-20261007.inc" fs/snapshot-impl.inc
cp "$stage/process-shared-20261007.c" memory/process.c
cp "$stage/build-shared-20261007.sh" build.sh
cp "$stage/elf-shared-memory-20261007.c" user/elf-shared-memory.c
cp "$stage/test_shared_memory_20261007.py" tests/test_shared_memory.py
PROCESS_TESTS=1 bash build.sh >shared-memory-build.log 2>&1
python3 tests/test_shared_memory.py >shared-memory-bios.log 2>&1
python3 tools/build_uefi.py >shared-memory-uefi-build.log 2>&1
python3 tests/test_shared_memory.py --uefi >shared-memory-uefi.log 2>&1
tail -n 3 shared-memory-bios.log
tail -n 3 shared-memory-uefi.log
