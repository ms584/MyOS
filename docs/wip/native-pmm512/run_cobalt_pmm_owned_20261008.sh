#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat /home/xytron404/pmm-syscall-R4sQZZ/pmm-syscall.status)" = TERMINAL_EXIT=0
test "$(cat cobalt-fatal-cache.status)" = TERMINAL_EXIT=0
cp "$stage/syscall-cobalt-pmm-owned-20261008.c" kernel/syscall.c
# Keep real allocation failure traces, remove high-volume successful read logs.
cp "$stage/runtime-cobalt-allocation-trace-20261008.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-pmm-owned.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-pmm-owned.status' EXIT
bash build.sh >cobalt-pmm-owned-build.log 2>&1
python3 tests/test_cobalt_native_visual_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-pmm-owned.log 2>&1
tail -n 5 cobalt-pmm-owned.log
