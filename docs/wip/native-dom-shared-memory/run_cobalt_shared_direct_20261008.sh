#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/shm-8m-4hJG2I
test "$(cat "$clean/shm-direct.status")" = TERMINAL_EXIT=0
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-relative-input.status)" = TERMINAL_EXIT=0
cp "$clean/include/shm-backing.h" include/shm-backing.h
cp "$stage/process-cobalt-shared-direct-diagnostic-20261008.c" memory/process.c
printf 'RUNNING\n' >cobalt-shared-direct.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-shared-direct.status' EXIT
bash build.sh >cobalt-shared-direct-build.log 2>&1
python3 tests/test_cobalt_native_http_relative_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-shared-direct.log 2>&1
tail -n 5 cobalt-shared-direct.log
