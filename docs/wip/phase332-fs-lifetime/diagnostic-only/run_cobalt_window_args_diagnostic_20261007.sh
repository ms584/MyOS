#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat /home/xytron404/cpu-clock-HEdtNZ/window-args.status)" = TERMINAL_EXIT=0
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat cobalt-cpu-clock-diagnostic.status)" = TERMINAL_EXIT=0
cp "$stage/process-window-args-20261007.c" memory/process.c
cp "$stage/syscall-window-args-20261007.c" kernel/syscall.c
printf 'RUNNING\n' >cobalt-window-args-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-window-args-diagnostic.status' EXIT
bash build.sh >cobalt-window-args-diagnostic-build.log 2>&1
python3 tests/test_startup_diagnostic.py build/cobalt-resources-dwxalc2n >cobalt-window-args-diagnostic.log 2>&1
tail -n 30 cobalt-window-args-diagnostic.log
