#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat pmm-syscall.status)" = TERMINAL_EXIT=0
printf 'WAITING_FOR_EXISTING_PMM_FULL_PID345602\n' >pmm-syscall-full.status
while test -r /proc/345602/cmdline && grep -aqF run_pmm512_full_20261008.sh /proc/345602/cmdline; do sleep 5; done
printf 'RUNNING\n' >pmm-syscall-full.status
trap 'result=$?; bash build.sh >pmm-syscall-full-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >pmm-syscall-full-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >pmm-syscall-full.status' EXIT
PROCESS_TESTS=1 bash build.sh >pmm-syscall-full-build.log 2>&1
python3 tests/test_system.py >pmm-syscall-full.log 2>&1
bash build.sh >pmm-syscall-full-normal.log 2>&1
for name in pmm_512m pmm_syscall shared_memory vm_hint image_protect server; do
 python3 "tests/test_${name}.py" >"pmm-syscall-full-${name}-bios.log" 2>&1
done
python3 tools/build_uefi.py >pmm-syscall-full-uefi-build.log 2>&1
for name in pmm_512m pmm_syscall shared_memory vm_hint image_protect server; do
 python3 "tests/test_${name}.py" --uefi >"pmm-syscall-full-${name}-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >pmm-syscall-full-normal.sha256
printf 'TERMINAL_EXIT=0\n' >pmm-syscall-full.status
trap - EXIT
