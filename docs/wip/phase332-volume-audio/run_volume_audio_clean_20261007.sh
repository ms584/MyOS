#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
main=/home/xytron404/myos
cd /home/xytron404/cd-QJYMwG
test "$(cat shm-trace.status)" = 'TERMINAL_EXIT=0'
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >volume-audio-clean.status' EXIT
# No abort probes, logging wrappers or engine modification in this build.
cp "$main/build.sh" build.sh
cp "$main/user/platform-system.c" user/platform-system.c
cp "$stage/runtime-statvfs-clean-20261007.c" user/platform-runtime.c
cp "$stage/exports-volume-audio-clean-20261007.py" tools/make_runtime_exports.py
PROCESS_TESTS=0 bash build.sh >volume-audio-clean-build.log 2>&1
test "$(nm -g build/platform-runtime.elf | grep -c MyOSMissingImport || true)" = 0
python3 tests/test_statvfs.py >volume-clean-bios.log 2>&1
python3 tests/test_audio_output.py >audio-clean-bios.log 2>&1
python3 tools/build_uefi.py >volume-audio-clean-uefi-build.log 2>&1
python3 tests/test_statvfs.py --uefi >volume-clean-uefi.log 2>&1
python3 tests/test_audio_output.py --uefi >audio-clean-uefi.log 2>&1
tail -n 1 volume-clean-bios.log
tail -n 1 audio-clean-bios.log
tail -n 1 volume-clean-uefi.log
tail -n 1 audio-clean-uefi.log
