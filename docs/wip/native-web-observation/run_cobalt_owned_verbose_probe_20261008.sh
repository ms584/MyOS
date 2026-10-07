#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-allocation-probe.status)" = TERMINAL_EXIT=0
cp cobalt-allocation-probe.log cobalt-allocation-probe-before-owned-path.log
cp cobalt-allocation-probe.status cobalt-allocation-probe-before-owned-path.status
cp "$stage/runtime-cobalt-allocation-trace-20261008.c" user/platform-runtime.c
cp "$stage/process-cobalt-http-verbose-diagnostic-20261008.c" memory/process.c
cp "$stage/syscall-cobalt-diagnostic-owned-path-20261008.c" kernel/syscall.c
printf 'RUNNING\n' >cobalt-owned-verbose-probe.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-owned-verbose-probe.status' EXIT
bash build.sh >cobalt-owned-verbose-probe-build.log 2>&1
python3 tests/test_cobalt_native_http_policy_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-owned-verbose-probe.log 2>&1
tail -n 5 cobalt-owned-verbose-probe.log
