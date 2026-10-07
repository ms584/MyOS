#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/shm-8m-4hJG2I
test "$(cat shm-direct.status)" = TERMINAL_EXIT=0
printf 'RUNNING\n' >shm-direct-full.status
trap 'result=$?; bash build.sh >shm-direct-full-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >shm-direct-full-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >shm-direct-full.status' EXIT
PROCESS_TESTS=1 bash build.sh >shm-direct-full-build.log 2>&1
python3 tests/test_system.py >shm-direct-full.log 2>&1
bash build.sh >shm-direct-full-normal.log 2>&1
python3 "$stage/prepare_platform_fonts_20261007.py" "$PWD" >shm-direct-full-font-package.log 2>&1
for name in shared_memory server fs_capacity vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" >"shm-direct-full-${name}-bios.log" 2>&1
done
python3 tools/build_uefi.py >shm-direct-full-uefi-build.log 2>&1
for name in shared_memory server fs_capacity vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" --uefi >"shm-direct-full-${name}-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >shm-direct-full-normal.sha256
printf 'TERMINAL_EXIT=0\n' >shm-direct-full.status
trap - EXIT
tail -n 3 shm-direct-full.log
