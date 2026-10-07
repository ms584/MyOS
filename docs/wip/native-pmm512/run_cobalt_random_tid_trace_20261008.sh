#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-random-eof-trace.status)" = TERMINAL_EXIT=0
cp "$stage/runtime-cobalt-random-read-trace-20261008.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-random-tid-trace.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-random-tid-trace.status' EXIT
bash build.sh >cobalt-random-tid-trace-build.log 2>&1
python3 tests/test_cobalt_native_visual_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-random-tid-trace.log 2>&1
tail -n 5 cobalt-random-tid-trace.log
