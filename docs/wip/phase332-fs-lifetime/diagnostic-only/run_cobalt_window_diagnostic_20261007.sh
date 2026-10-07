#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat fs-cobalt-fatal-trace.status)" = TERMINAL_EXIT=0
cp "$stage/platform-window-diagnostic-20261007.c" user/platform-window.c
printf 'RUNNING\n' >cobalt-window-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-window-diagnostic.status' EXIT
bash build.sh >cobalt-window-diagnostic-build.log 2>&1
python3 tests/test_startup_diagnostic.py build/cobalt-resources-dwxalc2n >cobalt-window-diagnostic.log 2>&1
tail -n 30 cobalt-window-diagnostic.log
