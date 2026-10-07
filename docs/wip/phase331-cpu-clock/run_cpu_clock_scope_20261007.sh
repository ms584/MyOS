#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/fs-native-VzEALo
main=/home/xytron404/myos
probe=$(mktemp -d /home/xytron404/cpu-clock-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$stage/build-fs-lifetime-20261007.sh" "$probe/build.sh"
cp "$stage/process-cpu-clock-20261007.h" "$probe/include/process.h"
cp "$stage/scheduler-cpu-clock-20261007.c" "$probe/cpu/scheduler.c"
cp "$stage/syscall-fs-cpu-clock-20261007.c" "$probe/kernel/syscall.c"
cp "$stage/elf-cpu-clock-20261007.c" "$probe/user/elf-cpu-clock.c"
cp "$stage/test_cpu_clock_20261007.py" "$probe/tests/test_cpu_clock.py"
cd "$probe";mkdir -p build
printf 'CPU_CLOCK_TREE=%s\n' "$probe"
printf 'RUNNING\n' >cpu-clock.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cpu-clock.status' EXIT
bash build.sh >cpu-clock-build.log 2>&1
python3 tests/test_cpu_clock.py >cpu-clock-bios.log 2>&1
python3 tools/build_uefi.py >cpu-clock-uefi-build.log 2>&1
python3 tests/test_cpu_clock.py --uefi >cpu-clock-uefi.log 2>&1
tail -n 1 cpu-clock-bios.log
tail -n 1 cpu-clock-uefi.log
