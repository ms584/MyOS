#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-random-tid-trace.status)" = TERMINAL_EXIT=0
cp "$stage/signal-cobalt-fatal-cache-20261008.c" kernel/signal.c
printf 'RUNNING\n' >cobalt-fatal-cache.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-fatal-cache.status' EXIT
bash build.sh >cobalt-fatal-cache-build.log 2>&1
python3 tests/test_cobalt_native_visual_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-fatal-cache.log 2>&1
tail -n 5 cobalt-fatal-cache.log
