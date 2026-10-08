#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat shm-inode-pool-full.status)" = TERMINAL_EXIT=2
# The focused filesystem suite passed. Its wrapper used a nonexistent log
# filename in a postcondition; retain that failure and validate the real log.
cp shm-inode-pool-full.status shm-inode-pool-postcondition-failure.status
grep -F 'PASS: independent SHM inode pool, snapshot64 compatibility and exact resources' build/system-tests/framebuffer-64M.log
printf 'RUNNING\n' >shm-inode-pool-full.status
trap 'result=$?; bash build.sh >shm-inode-pool-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >shm-inode-pool-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >shm-inode-pool-full.status' EXIT
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
