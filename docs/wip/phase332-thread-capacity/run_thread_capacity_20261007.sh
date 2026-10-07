#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
main=/home/xytron404/myos
cd /home/xytron404/cd-QJYMwG
test "$(cat shared-crash-trace.status)" = 'TERMINAL_EXIT=0'
if test -f thread-capacity-tls.log;then cp thread-capacity-tls.log thread-capacity-tls-old-guard.log;fi
printf 'RUNNING\n' >thread-capacity.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >thread-capacity.status' EXIT
cp "$stage/process-capacity-20261007.h" include/process.h
cp "$stage/scheduler-capacity-20261007.c" cpu/scheduler.c
cp "$stage/thread-limit.h" include/
cp "$stage/elf-platform-system-capacity-20261007.c" user/elf-platform-system.c
cp "$stage/elf-thread-capacity-20261007.c" user/elf-thread-capacity.c
cp "$stage/test_thread_capacity_20261007.py" tests/test_thread_capacity.py
cp "$stage/elf-native-tls-capacity-20261007.c" user/elf-native-tls.c
cp "$stage/test_native_tls_capacity_20261007.py" tests/test_native_tls.py
cp "$stage/build-shared-20261007.sh" build.sh
cp "$stage/runtime-statvfs-clean-20261007.c" user/platform-runtime.c
cp "$stage/exports-volume-audio-clean-20261007.py" tools/make_runtime_exports.py
cp "$main/user/platform-system.c" user/platform-system.c
PROCESS_TESTS=1 bash build.sh >thread-capacity-build.log 2>&1
python3 tests/test_thread_capacity.py >thread-capacity-bios.log 2>&1
python3 tests/test_native_tls.py >thread-capacity-tls.log 2>&1
python3 tests/test_thread_services.py >thread-capacity-services.log 2>&1
python3 tools/build_uefi.py >thread-capacity-uefi-build.log 2>&1
python3 tests/test_thread_capacity.py --uefi >thread-capacity-uefi.log 2>&1
tail -n 2 thread-capacity-bios.log
tail -n 2 thread-capacity-uefi.log
