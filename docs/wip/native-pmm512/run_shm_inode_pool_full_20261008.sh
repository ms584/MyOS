#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
main=/home/xytron404/myos
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat pmm-syscall-full.status)" = TERMINAL_EXIT=0
for file in include/fs.h fs/ramfs.c fs/snapshot-impl.inc fs/fs-test.c memory/process.c memory/memory.c kernel/syscall.c user/platform-runtime.c; do cmp "$main/$file" "$file"; done
cp "$stage/fs-shm-pool-20261008.h" include/fs.h
cp "$stage/ramfs-shm-pool-20261008.c" fs/ramfs.c
cp "$stage/snapshot-shm-pool-20261008.inc" fs/snapshot-impl.inc
cp "$stage/fs-test-shm-pool-20261008.c" fs/fs-test.c
printf 'RUNNING\n' >shm-inode-pool-full.status
trap 'result=$?; bash build.sh >shm-inode-pool-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >shm-inode-pool-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >shm-inode-pool-full.status' EXIT
python3 tests/test_system.py --suite filesystem >shm-inode-pool-filesystem.log 2>&1
grep -F 'PASS: independent SHM inode pool, snapshot64 compatibility and exact resources' build/system-tests/framebuffer-64M.log
python3 tests/test_system.py >shm-inode-pool-full.log 2>&1
bash build.sh >shm-inode-pool-normal.log 2>&1
for scope in shared_memory statvfs fs_runtime_lifetime; do
 python3 "tests/test_$scope.py" >"shm-inode-pool-$scope-bios.log" 2>&1
done
python3 tools/build_uefi.py >shm-inode-pool-uefi-build.log 2>&1
for scope in shared_memory statvfs fs_runtime_lifetime; do
 python3 "tests/test_$scope.py" --uefi >"shm-inode-pool-$scope-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >shm-inode-pool-normal.sha256
printf 'TERMINAL_EXIT=0\n' >shm-inode-pool-full.status
trap - EXIT
