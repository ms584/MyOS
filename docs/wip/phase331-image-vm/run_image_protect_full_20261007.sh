#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/image-protect-TSMFT6
test "$(cat image-protect.status)" = TERMINAL_EXIT=0
for name in raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 cp "$stage/test_${name}_20261007.py" "tests/test_${name}.py"
done
printf 'RUNNING\n' >image-full.status
trap 'result=$?; bash build.sh >image-full-normal-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >image-full.status' EXIT
PROCESS_TESTS=1 bash build.sh >image-full-build.log 2>&1
python3 tests/test_system.py >image-full.log 2>&1
bash build.sh >image-full-normal.log 2>&1
python3 "$stage/prepare_platform_fonts_20261007.py" "$PWD" >image-full-font-package.log 2>&1
for name in image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" >"image-full-${name}-bios.log" 2>&1
done
python3 tools/build_uefi.py >image-full-uefi-build.log 2>&1
for name in image_protect raw_clock cpu_clock platform_fonts fs_unavailable player_unavailable; do
 python3 "tests/test_${name}.py" --uefi >"image-full-${name}-uefi.log" 2>&1
done
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >image-full-normal.sha256
printf 'TERMINAL_EXIT=0\n' >image-full.status
trap - EXIT
tail -n 5 image-full.log
