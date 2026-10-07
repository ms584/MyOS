#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
main=/home/xytron404/myos
cd /home/xytron404/cd-QJYMwG
test "$(cat capacity32.status)" = 'TERMINAL_EXIT=0'
test "$(cat shared-clean-full.status)" = 'TERMINAL_EXIT=1'
cp shared-clean-full.log shared-clean-full-pre-audio-bound-failure.log
cp shared-clean-full.status shared-clean-full-pre-audio-bound-failure.status
cp "$stage/elf-audio-sink-capacity32.c" user/elf-audio-sink.c
printf 'RUNNING\n' >shared-clean-full.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >shared-clean-full.status' EXIT
# Source tree becomes frozen until this entire sequence is terminal.
cp "$stage/build-shared-20261007.sh" build.sh
cp "$stage/runtime-statvfs-clean-20261007.c" user/platform-runtime.c
cp "$stage/exports-volume-audio-clean-20261007.py" tools/make_runtime_exports.py
cp "$main/user/platform-system.c" user/platform-system.c
cp "$stage/process-shared-20261007.c" memory/process.c
PROCESS_TESTS=1 bash build.sh >shared-clean-build.log 2>&1
test "$(nm -g build/platform-runtime.elf | grep -c MyOSMissingImport || true)" = 0
python3 tests/test_system.py >shared-clean-full.log 2>&1
python3 tests/test_thread_capacity.py >shared-clean-capacity-bios.log 2>&1
python3 tests/test_shared_memory.py >shared-clean-scope-bios.log 2>&1
python3 tests/test_statvfs.py >shared-clean-volume-bios.log 2>&1
python3 tests/test_audio_output.py >shared-clean-audio-bios.log 2>&1
python3 tests/test_uname_abi.py >shared-clean-uname-bios.log 2>&1
python3 tools/build_uefi.py >shared-clean-uefi-build.log 2>&1
python3 tests/test_shared_memory.py --uefi >shared-clean-scope-uefi.log 2>&1
python3 tests/test_thread_capacity.py --uefi >shared-clean-capacity-uefi.log 2>&1
python3 tests/test_statvfs.py --uefi >shared-clean-volume-uefi.log 2>&1
python3 tests/test_audio_output.py --uefi >shared-clean-audio-uefi.log 2>&1
python3 tests/test_audio_sink.py --uefi >shared-clean-audio-sink-uefi.log 2>&1
python3 tests/test_uname_abi.py --uefi >shared-clean-uname-uefi.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >shared-clean-final.sha256
sha256sum -c /home/xytron404/fm-prC903/main-before.sha256
tail -n 4 shared-clean-full.log
