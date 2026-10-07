#!/usr/bin/env bash
set -euo pipefail
candidate=/home/xytron404/pmm-512m-01MfM9
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat "$candidate/pmm512.status")" = TERMINAL_EXIT=0
test "$(cat cobalt-visual-probe.status)" = TERMINAL_EXIT=0
cp "$candidate/memory/memory.c" memory/memory.c
printf 'RUNNING\n' >cobalt-pmm512-visual.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-pmm512-visual.status' EXIT
bash build.sh >cobalt-pmm512-visual-build.log 2>&1
python3 tests/test_cobalt_native_visual_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-pmm512-visual.log 2>&1
tail -n 5 cobalt-pmm512-visual.log
