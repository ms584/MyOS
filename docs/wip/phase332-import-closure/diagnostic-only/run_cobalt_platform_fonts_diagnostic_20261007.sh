#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
fonts=/home/xytron404/platform-fonts-01W7Ri
test "$(cat "$fonts/platform-fonts.status")" = TERMINAL_EXIT=0
cd /home/xytron404/fs-cobalt-diag-VQzrRc
test "$(cat cobalt-window-args-diagnostic.status)" = TERMINAL_EXIT=0
cp "$stage/syscall-platform-fonts-20261007.c" kernel/syscall.c
cp "$stage/test_startup_fonts_diagnostic_20261007.py" tests/test_startup_fonts_diagnostic.py
printf 'RUNNING\n' >cobalt-platform-fonts-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-platform-fonts-diagnostic.status' EXIT
bash build.sh >cobalt-platform-fonts-diagnostic-build.log 2>&1
python3 tests/test_startup_fonts_diagnostic.py build/cobalt-resources-dwxalc2n "$fonts/build/platform-fonts-20261007" >cobalt-platform-fonts-diagnostic.log 2>&1
tail -n 30 cobalt-platform-fonts-diagnostic.log
