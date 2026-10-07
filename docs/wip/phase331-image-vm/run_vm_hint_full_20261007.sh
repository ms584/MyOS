#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/vm-hint-PznuHf
test "$(cat vm-hint.status)" = TERMINAL_EXIT=0
for name in raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 cp "$stage/test_${name}_20261007.py" "tests/test_${name}.py"
done
printf 'RUNNING\n' >vm-hint-full.status
trap 'result=$?; bash build.sh >vm-hint-full-normal-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >vm-hint-full.status' EXIT
PROCESS_TESTS=1 bash build.sh >vm-hint-full-build.log 2>&1
python3 tests/test_system.py >vm-hint-full.log 2>&1
bash build.sh >vm-hint-full-normal.log 2>&1
python3 "$stage/prepare_platform_fonts_20261007.py" "$PWD" >vm-hint-full-font-package.log 2>&1
for name in vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" >"vm-hint-full-${name}-bios.log" 2>&1
done
python3 tools/build_uefi.py >vm-hint-full-uefi-build.log 2>&1
for name in vm_hint image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" --uefi >"vm-hint-full-${name}-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >vm-hint-full-normal.sha256
printf 'TERMINAL_EXIT=0\n' >vm-hint-full.status
trap - EXIT
tail -n 5 vm-hint-full.log
