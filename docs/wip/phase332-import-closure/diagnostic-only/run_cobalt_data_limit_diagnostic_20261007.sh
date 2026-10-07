#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
fonts=/home/xytron404/platform-fonts-01W7Ri
test "$(cat "$fonts/data-limit.status")" = TERMINAL_EXIT=0
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat cobalt-platform-fonts-diagnostic.status)" = TERMINAL_EXIT=0
cp "$stage/runtime-data-limit-20261007.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-data-limit-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-data-limit-diagnostic.status' EXIT
bash build.sh >cobalt-data-limit-diagnostic-build.log 2>&1
python3 tests/test_startup_fonts_diagnostic.py build/cobalt-resources-dwxalc2n "$fonts/build/platform-fonts-20261007" >cobalt-data-limit-diagnostic.log 2>&1
tail -n 35 cobalt-data-limit-diagnostic.log
