#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-image-fault-trace.status)" = TERMINAL_EXIT=0
cp "$stage/runtime-vm-map-trace-20261007.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-vm-map-trace.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-vm-map-trace.status' EXIT
bash build.sh >cobalt-vm-map-trace-build.log 2>&1
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-vm-map-trace.log 2>&1
grep 'DIAGNOSTIC mmap' build/system-tests/cobalt-startup-diagnostic.log
tail -n 5 cobalt-vm-map-trace.log
