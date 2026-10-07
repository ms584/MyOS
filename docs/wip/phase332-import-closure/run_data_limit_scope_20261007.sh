#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/platform-fonts-01W7Ri
test "$(cat platform-fonts.status)" = TERMINAL_EXIT=0
cp "$stage/runtime-data-limit-20261007.c" user/platform-runtime.c
cp "$stage/elf-data-limit-20261007.c" user/elf-data-limit.c
cp "$stage/test_data_limit_20261007.py" tests/test_data_limit.py
printf 'RUNNING\n' >data-limit.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >data-limit.status' EXIT
bash build.sh >data-limit-build.log 2>&1
python3 tests/test_data_limit.py >data-limit-bios.log 2>&1
python3 tools/build_uefi.py >data-limit-uefi-build.log 2>&1
python3 tests/test_data_limit.py --uefi >data-limit-uefi.log 2>&1
tail -n 1 data-limit-bios.log
tail -n 1 data-limit-uefi.log
