#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/player-unavailable-bI04LM
test "$(cat import-closure.status)" = TERMINAL_EXIT=0
cp "$stage/syscall-raw-clock-20261007.c" kernel/syscall.c
cp "$stage/elf-raw-clock-20261007.c" user/elf-raw-clock.c
cp "$stage/elf-cpu-clock-raw-20261007.c" user/elf-cpu-clock.c
cp "$stage/test_raw_clock_20261007.py" tests/test_raw_clock.py
cp "$stage/test_cpu_clock_20261007.py" tests/test_cpu_clock.py
printf 'RUNNING\n' >raw-clock.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >raw-clock.status' EXIT
bash build.sh >raw-clock-build.log 2>&1
python3 tests/test_raw_clock.py >raw-clock-bios.log 2>&1
python3 tests/test_cpu_clock.py >raw-clock-cpu-bios.log 2>&1
python3 tools/build_uefi.py >raw-clock-uefi-build.log 2>&1
python3 tests/test_raw_clock.py --uefi >raw-clock-uefi.log 2>&1
python3 tests/test_cpu_clock.py --uefi >raw-clock-cpu-uefi.log 2>&1
tail -n 1 raw-clock-bios.log
tail -n 1 raw-clock-uefi.log
