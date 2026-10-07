#!/usr/bin/env bash
set -euo pipefail
source_tree=/home/xytron404/cd-QJYMwG
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/myos
test "$(git rev-parse --short HEAD)" = 5809e1e
test -z "$(git status --porcelain)"
test "$(cat "$source_tree/shared-clean-full.status")" = TERMINAL_EXIT=0
test "$(nm -g "$source_tree/build/platform-runtime.elf" | grep -c MyOSMissingImport || true)" = 0
files=(build.sh include/fs-volume-call.h include/fs.h include/os.h include/process.h
 include/shm-backing.h include/syscall.h include/thread-limit.h include/vm-area.h
 include/vm-file-call.h memory/memory.c memory/process.c cpu/scheduler.c fs/fs-test.c
 fs/ramfs.c fs/shm-backing.c fs/snapshot-impl.inc kernel/syscall.c user/elf-audio-output.c
 user/elf-audio-sink.c user/elf-file-map.c user/elf-native-tls.c user/elf-platform-system.c
 user/elf-shared-memory.c user/elf-sparse-vm.c user/elf-statvfs.c user/elf-thread-capacity.c
 user/platform-media-policy.c user/platform-runtime.c user/sdk/sys/statvfs.h
 tests/test_audio_output.py tests/test_native_tls.py tests/test_pmm_refs.py
 tests/test_shared_memory.py tests/test_statvfs.py tests/test_thread_capacity.py
 tests/test_uname_abi.py tools/make_runtime_exports.py)
for item in "${files[@]}"; do
 test -f "$source_tree/$item"
 mkdir -p "$(dirname "$item")"
 cp "$source_tree/$item" "$item"
 cmp "$source_tree/$item" "$item"
done
printf 'RUNNING\n' >build/clean-prerequisites-promotion.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >build/clean-prerequisites-promotion.status' EXIT
bash build.sh >build/clean-prerequisites-build.log 2>&1
cmp build/kernel.bin "$source_tree/build/kernel.bin"
cmp build/platform-runtime.bin "$source_tree/build/platform-runtime.bin"
test "$(nm -g build/platform-runtime.elf | grep -c MyOSMissingImport || true)" = 0
python3 tests/test_statvfs.py >build/clean-prerequisites-volume-bios.log 2>&1
python3 tests/test_audio_output.py >build/clean-prerequisites-audio-bios.log 2>&1
python3 tests/test_shared_memory.py >build/clean-prerequisites-shared-bios.log 2>&1
python3 tests/test_thread_capacity.py >build/clean-prerequisites-capacity-bios.log 2>&1
python3 tests/test_uname_abi.py >build/clean-prerequisites-uname-bios.log 2>&1
python3 tools/build_uefi.py >build/clean-prerequisites-uefi-build.log 2>&1
python3 tests/test_shared_memory.py --uefi >build/clean-prerequisites-shared-uefi.log 2>&1
python3 tests/test_statvfs.py --uefi >build/clean-prerequisites-volume-uefi.log 2>&1
python3 tests/test_audio_output.py --uefi >build/clean-prerequisites-audio-uefi.log 2>&1
python3 tests/test_thread_capacity.py --uefi >build/clean-prerequisites-capacity-uefi.log 2>&1
python3 tests/test_uname_abi.py --uefi >build/clean-prerequisites-uname-uefi.log 2>&1
python3 tests/test_audio_sink.py --uefi >build/clean-prerequisites-sink-uefi.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >build/clean-prerequisites-normal.sha256
git diff --check
printf 'PASS clean source/binary equivalence + production scopes; private full27417 reused unchanged.\n'
