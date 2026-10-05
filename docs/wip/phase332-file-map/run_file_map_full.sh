#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /tmp/myos-file-map-u9NQ2N
test -f third_party/compiler-rt/emutls.c
cp "$stage/elf-sparse-vm.c" user/
cp "$stage/test_sparse_vm.py" tests/
full_log=${FILE_MAP_FULL_LOG:-file-map-full.log}
python3 tests/test_system.py >"$full_log" 2>&1 || { tail -n 60 "$full_log";exit 1; }
tail -n 4 "$full_log"
python3 tools/build_uefi.py >file-map-final-uefi.log 2>&1
python3 tests/test_file_map.py >file-map-final-bios.log 2>&1
tail -n 2 file-map-final-bios.log
python3 tests/test_file_map.py --uefi >file-map-final-uefi-native.log 2>&1
tail -n 2 file-map-final-uefi-native.log
python3 tests/test_sparse_vm.py >file-map-sparse-bios.log 2>&1
tail -n 2 file-map-sparse-bios.log
python3 tests/test_sparse_vm.py --no-entropy >file-map-sparse-no-entropy.log 2>&1
tail -n 2 file-map-sparse-no-entropy.log
python3 tests/test_sparse_vm.py --uefi >file-map-sparse-uefi.log 2>&1
tail -n 2 file-map-sparse-uefi.log
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img
