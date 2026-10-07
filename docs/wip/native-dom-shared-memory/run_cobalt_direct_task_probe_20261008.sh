#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-shared-direct.status)" = TERMINAL_EXIT=0
cp "$stage/cobalt_task_snapshot_20261008.py" tests/cobalt_task_snapshot.py
cp "$stage/test_cobalt_native_direct_task_probe_20261008.py" tests/test_cobalt_native_direct_task_probe.py
gcc -m64 -std=c11 -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude "$stage/cobalt_task_layout_20261008.c" -o build/cobalt-task-layout
printf 'RUNNING\n' >cobalt-direct-task.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-direct-task.status' EXIT
python3 tests/test_cobalt_native_direct_task_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-direct-task.log 2>&1
tail -n 5 cobalt-direct-task.log
