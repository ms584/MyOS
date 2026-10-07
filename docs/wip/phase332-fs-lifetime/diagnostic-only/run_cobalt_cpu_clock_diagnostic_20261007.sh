#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat /home/xytron404/cpu-clock-HEdtNZ/cpu-clock.status)" = TERMINAL_EXIT=0
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat cobalt-window-diagnostic.status)" = TERMINAL_EXIT=0
cp "$stage/process-cpu-clock-20261007.h" include/process.h
cp "$stage/scheduler-cpu-clock-20261007.c" cpu/scheduler.c
cp "$stage/syscall-fs-cpu-clock-20261007.c" kernel/syscall.c
printf 'RUNNING\n' >cobalt-cpu-clock-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-cpu-clock-diagnostic.status' EXIT
bash build.sh >cobalt-cpu-clock-diagnostic-build.log 2>&1
python3 tests/test_startup_diagnostic.py build/cobalt-resources-dwxalc2n >cobalt-cpu-clock-diagnostic.log 2>&1
tail -n 30 cobalt-cpu-clock-diagnostic.log
