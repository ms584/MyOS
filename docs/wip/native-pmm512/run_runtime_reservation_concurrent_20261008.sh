#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat runtime-reservation.status)" = TERMINAL_EXIT=0
cp "$stage/elf-fs-runtime-reservation-20261008.c" user/elf-fs-runtime-lifetime.c
printf 'RUNNING\n' >runtime-reservation-concurrent.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >runtime-reservation-concurrent.status' EXIT
python3 tests/test_fs_runtime_lifetime.py >runtime-reservation-concurrent-bios.log 2>&1
cp build/system-tests/fs-runtime.log runtime-reservation-concurrent-bios-serial.txt
grep -c 'native runtime EMFILE no-create/no-truncate, rollback and concurrent publication PASS' runtime-reservation-concurrent-bios-serial.txt
python3 tests/test_fs_runtime_lifetime.py --uefi >runtime-reservation-concurrent-uefi.log 2>&1
cp build/system-tests/fs-runtime-uefi.log runtime-reservation-concurrent-uefi-serial.txt
grep -c 'native runtime EMFILE no-create/no-truncate, rollback and concurrent publication PASS' runtime-reservation-concurrent-uefi-serial.txt
tail -1 runtime-reservation-concurrent-bios.log
tail -1 runtime-reservation-concurrent-uefi.log
