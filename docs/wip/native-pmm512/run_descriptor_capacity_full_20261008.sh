#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat shm-inode-pool-full.status)" = TERMINAL_EXIT=0
test "$(cat /home/xytron404/myos/shm-inode-pool-promotion.status)" = TERMINAL_EXIT=0
sha256sum -c shm-inode-pool-normal.sha256
if pgrep -af qemu-system | grep -F '/home/xytron404/pmm-syscall-R4sQZZ/build/'; then
 echo 'Refusing live candidate mutation'; exit 1
fi
cp "$stage/fs-shm-pool-handles64-20261008.h" include/fs.h
cp "$stage/platform-runtime-reservation-clean-20261008.c" user/platform-runtime.c
cp "$stage/elf-fs-runtime-reservation-20261008.c" user/elf-fs-runtime-lifetime.c
cp "$stage/elf-fs-capacity-fd128-20261008.c" user/elf-fs-capacity.c
cp "$stage/elf-platform-system-fd128-20261008.c" user/elf-platform-system.c
cp "$stage/test_fs_capacity_fd128_20261008.py" tests/test_fs_capacity.py
printf 'RUNNING\n' >descriptor-capacity-full.status
trap 'result=$?; bash build.sh >descriptor-capacity-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >descriptor-capacity-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >descriptor-capacity-full.status' EXIT
bash build.sh >descriptor-capacity-build.log 2>&1
python3 tools/build_uefi.py >descriptor-capacity-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >descriptor-capacity-normal.sha256
for firmware in bios uefi; do
 options=();if [[ "$firmware" == uefi ]]; then options=(--uefi);fi
 for scope in fs_capacity fs_runtime_lifetime statvfs shared_memory server; do
  python3 "tests/test_$scope.py" "${options[@]}" >"descriptor-capacity-$scope-$firmware.log" 2>&1
 done
done
python3 tests/test_system.py >descriptor-capacity-full.log 2>&1
bash build.sh >descriptor-capacity-normal.log 2>&1
python3 tools/build_uefi.py >descriptor-capacity-uefi-normal.log 2>&1
sha256sum -c descriptor-capacity-normal.sha256
printf 'TERMINAL_EXIT=0\n' >descriptor-capacity-full.status
trap - EXIT
