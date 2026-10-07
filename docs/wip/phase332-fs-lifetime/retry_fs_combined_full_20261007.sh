#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-native-VzEALo
test "$(cat fs-full.status)" = TERMINAL_EXIT=1
test "$(cat /home/xytron404/cpu-clock-HEdtNZ/window-args.status)" = TERMINAL_EXIT=0
cp fs-full.log fs-full-missing-pinned-header-failure.log
cp fs-full.status fs-full-missing-pinned-header-failure.status
# No missing-call probes, trace builds or diagnostic signal code are copied.
cp "$stage/process-cpu-clock-20261007.h" include/process.h
cp "$stage/scheduler-cpu-clock-20261007.c" cpu/scheduler.c
cp "$stage/syscall-window-args-20261007.c" kernel/syscall.c
cp "$stage/process-window-args-20261007.c" memory/process.c
cp "$stage/elf-starboard-window-args-20261007.c" user/elf-starboard-launcher.c
cp "$stage/elf-cpu-clock-20261007.c" user/elf-cpu-clock.c
cp "$stage/test_cpu_clock_20261007.py" tests/test_cpu_clock.py
test -L build/engine-evaluation || ln -s /home/xytron404/myos/build/engine-evaluation build/engine-evaluation
test -f build/engine-evaluation/contracts/window.h
exec bash "$stage/run_fs_full_regression_20261007.sh"
