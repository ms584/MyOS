#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat descriptor-capacity-full.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/pmm-syscall-R4sQZZ/build/'; then
 echo 'Refusing live candidate mutation'; exit 1
fi
cp descriptor-capacity-full.log descriptor-capacity-lowmem-failure.log
cp descriptor-capacity-full.status descriptor-capacity-lowmem-failure.status
cp "$stage/process-fs-table-heap-20261008.h" include/process.h
cp "$stage/process-fs-table-heap-20261008.c" memory/process.c
printf 'RUNNING\n' >descriptor-heap-full.status
trap 'result=$?; bash build.sh >descriptor-heap-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >descriptor-heap-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >descriptor-heap-full.status' EXIT
bash build.sh >descriptor-heap-build.log 2>&1
python3 tools/build_uefi.py >descriptor-heap-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >descriptor-heap-normal.sha256
for firmware in bios uefi; do
 options=();if [[ "$firmware" == uefi ]]; then options=(--uefi);fi
 for scope in fs_capacity fs_runtime_lifetime statvfs shared_memory server; do
  python3 "tests/test_$scope.py" "${options[@]}" >"descriptor-heap-$scope-$firmware.log" 2>&1
 done
done
python3 tests/test_system.py >descriptor-heap-full.log 2>&1
bash build.sh >descriptor-heap-normal.log 2>&1
python3 tools/build_uefi.py >descriptor-heap-uefi-normal.log 2>&1
sha256sum -c descriptor-heap-normal.sha256
printf 'TERMINAL_EXIT=0\n' >descriptor-heap-full.status
trap - EXIT
