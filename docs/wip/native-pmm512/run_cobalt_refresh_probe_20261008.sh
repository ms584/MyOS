#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-pmm-owned.status)" = TERMINAL_EXIT=0
cp "$stage/test_cobalt_native_refresh_probe_20261008.py" tests/test_cobalt_native_refresh_probe.py
printf 'RUNNING\n' >cobalt-refresh-probe.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-refresh-probe.status' EXIT
python3 tests/test_cobalt_native_refresh_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-refresh-probe.log 2>&1
tail -n 5 cobalt-refresh-probe.log
