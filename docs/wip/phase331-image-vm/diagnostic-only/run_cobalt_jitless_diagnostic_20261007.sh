#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-vm-protect-trace.status)" = TERMINAL_EXIT=0
cp "$stage/process-cobalt-jitless-diagnostic-20261007.c" memory/process.c
cp "$stage/runtime-vm-hint-20261007.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-jitless.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-jitless.status' EXIT
bash build.sh >cobalt-jitless-build.log 2>&1
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-jitless.log 2>&1
tail -n 32 cobalt-jitless.log
