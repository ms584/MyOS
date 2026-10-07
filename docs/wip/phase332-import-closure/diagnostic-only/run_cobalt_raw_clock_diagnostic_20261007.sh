#!/usr/bin/env bash
set -euo pipefail
clean=/home/xytron404/player-unavailable-bI04LM
test "$(cat "$clean/raw-clock.status")" = TERMINAL_EXIT=0
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-closed-imports.status)" = TERMINAL_EXIT=0
cp "$clean/kernel/syscall.c" kernel/syscall.c
printf 'RUNNING\n' >cobalt-raw-clock.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-raw-clock.status' EXIT
bash build.sh >cobalt-raw-clock-build.log 2>&1
if nm build/platform-runtime.elf | grep -q MyOSMissing; then exit 1; fi
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n "$clean/../platform-fonts-01W7Ri/build/platform-fonts-20261007" >cobalt-raw-clock.log 2>&1
tail -n 35 cobalt-raw-clock.log
