#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/pmm-512m-01MfM9
test "$(cat pmm512.status)" = TERMINAL_EXIT=0
printf 'WAITING_FOR_EXISTING_FULL_PID321159\n' >pmm512-full.status
# This exact runner PID and command were checked live before queuing. Avoid
# a third simultaneous QEMU regression, and never mutate the old source tree.
while test -r /proc/321159/cmdline && grep -aqF run_shm_direct_full_20261008.sh /proc/321159/cmdline; do sleep 5; done
printf 'RUNNING\n' >pmm512-full.status
trap 'result=$?; bash build.sh >pmm512-full-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >pmm512-full-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >pmm512-full.status' EXIT
PROCESS_TESTS=1 bash build.sh >pmm512-full-build.log 2>&1
python3 tests/test_system.py >pmm512-full.log 2>&1
bash build.sh >pmm512-full-normal.log 2>&1
python3 "$stage/prepare_platform_fonts_20261007.py" "$PWD" >pmm512-full-font-package.log 2>&1
for name in pmm_512m shared_memory server fs_capacity vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" >"pmm512-full-${name}-bios.log" 2>&1
done
python3 tools/build_uefi.py >pmm512-full-uefi-build.log 2>&1
for name in pmm_512m shared_memory server fs_capacity vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" --uefi >"pmm512-full-${name}-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >pmm512-full-normal.sha256
printf 'TERMINAL_EXIT=0\n' >pmm512-full.status
trap - EXIT
