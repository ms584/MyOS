#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat fs-cobalt-diag.status)" = TERMINAL_EXIT=0
cp "$stage/signal-cobalt-fatal-trace-20261007.c" kernel/signal.c
printf 'RUNNING\n' >fs-cobalt-fatal-trace.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >fs-cobalt-fatal-trace.status' EXIT
bash build.sh >fs-cobalt-fatal-trace-build.log 2>&1
python3 tests/test_startup_diagnostic.py build/cobalt-resources-dwxalc2n >fs-cobalt-fatal-trace.log 2>&1
tail -n 30 fs-cobalt-fatal-trace.log
