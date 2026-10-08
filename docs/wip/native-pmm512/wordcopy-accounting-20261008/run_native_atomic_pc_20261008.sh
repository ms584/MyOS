#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-gs-errno.status)" = TERMINAL_EXIT=0
backup=$(mktemp build/gpu-observer-backup-XXXXXX.py)
cp tests/cobalt_gpu_frame_observer.py "$backup"
trap 'result=$?; cp "$backup" tests/cobalt_gpu_frame_observer.py; printf "TERMINAL_EXIT=%s\n" "$result" >native-atomic-pc.status' EXIT
printf 'RUNNING\n' >native-atomic-pc.status
gcc -std=c11 -Iinclude "$stage/cobalt_gpu_frame_layout_20261008.c" -o build/cobalt-gpu-frame-layout
cp "$stage/cobalt_atomic_pc_observer_20261008.py" tests/cobalt_gpu_frame_observer.py
MYOS_TLS_GPU_FRAMES=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-atomic-pc-interactive.log 2>&1
