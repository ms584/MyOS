#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/vm-hint-PznuHf
test "$(cat "$clean/vm-hint.status")" = TERMINAL_EXIT=0
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-vm-map-trace.status)" = TERMINAL_EXIT=0
cp "$stage/process-vm-hint-fault-trace-20261007.c" memory/process.c
cp "$clean/include/process.h" include/process.h
cp "$clean/include/syscall.h" include/syscall.h
cp "$clean/kernel/syscall.c" kernel/syscall.c
cp "$stage/runtime-vm-hint-map-trace-20261007.c" user/platform-runtime.c
printf 'RUNNING\n' >cobalt-vm-hint.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-vm-hint.status' EXIT
bash build.sh >cobalt-vm-hint-build.log 2>&1
if nm build/platform-runtime.elf | grep -q MyOSMissing; then exit 1; fi
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-vm-hint.log 2>&1
grep 'DIAGNOSTIC mmap' build/system-tests/cobalt-startup-diagnostic.log
tail -n 30 cobalt-vm-hint.log
