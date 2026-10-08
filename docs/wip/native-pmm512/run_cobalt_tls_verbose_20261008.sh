#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-tls-io-trace.status)" = TERMINAL_EXIT=0
cp "$stage/process-cobalt-verified-tls-20261008.c" memory/process.c
cp "$stage/syscall-cobalt-tls-verbose-20261008.c" kernel/syscall.c
printf 'RUNNING\n' >cobalt-tls-verbose.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-tls-verbose.status' EXIT
bash build.sh >cobalt-tls-verbose-build.log 2>&1
MYOS_TLS_TASK_SNAPSHOT=1 python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-tls-verbose.log 2>&1
