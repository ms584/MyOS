#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-native-VzEALo
test "$(cat fs-full.status)" = TERMINAL_EXIT=1
cp fs-full.log fs-full-old-argc-fixture-failure.log
cp fs-full.status fs-full-old-argc-fixture-failure.status
cp "$stage/elf-system-path-window-args-20261007.c" user/elf-system-path.c
printf 'RUNNING\n' >launcher-path.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >launcher-path.status' EXIT
python3 tests/test_starboard_system.py --launcher >launcher-path-bios.log 2>&1
python3 tools/build_uefi.py >launcher-path-uefi-build.log 2>&1
python3 tests/test_starboard_system.py --launcher --uefi >launcher-path-uefi.log 2>&1
tail -n 1 launcher-path-bios.log
tail -n 1 launcher-path-uefi.log
printf 'TERMINAL_EXIT=0\n' >launcher-path.status
trap - EXIT
bash "$stage/run_fs_full_regression_20261007.sh"
