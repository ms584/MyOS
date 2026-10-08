#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
# This native experiment is terminal; the independent full-suite tree is frozen.
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing to mutate a live VM tree'; exit 1
fi
cp "$stage/elf-fs-runtime-reservation-20261008.c" user/elf-fs-runtime-lifetime.c
cp /home/xytron404/myos/tests/test_fs_runtime_lifetime.py tests/test_fs_runtime_lifetime.py
if [[ -f runtime-reservation-before.log ]]; then
 cp runtime-reservation-before.log runtime-reservation-missing-fixture.log
fi
printf 'RUNNING\n' >runtime-reservation.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >runtime-reservation.status' EXIT
# First retain a regression against the unchanged runtime binary.
set +e
python3 tests/test_fs_runtime_lifetime.py >runtime-reservation-before.log 2>&1
before=$?
set -e
test "$before" -ne 0
grep -F 'fstatat(AT_FDCWD,"/home/runtime-reserve-new",&st,0)==-1&&errno==2' runtime-reservation-before.log
cp "$stage/platform-runtime-reservation-20261008.c" user/platform-runtime.c
bash build.sh >runtime-reservation-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py >runtime-reservation-bios.log 2>&1
python3 tools/build_uefi.py >runtime-reservation-uefi-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py --uefi >runtime-reservation-uefi.log 2>&1
tail -1 runtime-reservation-bios.log
tail -1 runtime-reservation-uefi.log
