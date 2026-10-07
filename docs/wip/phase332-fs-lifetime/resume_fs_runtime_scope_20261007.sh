#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-native-VzEALo
test "$(cat fs-runtime.status)" = TERMINAL_EXIT=1
failure_stamp=$(date +%Y%m%d-%H%M%S)
cp fs-runtime-bios.log "fs-runtime-failure-$failure_stamp.log"
cp "$stage/elf-fs-runtime-lifetime-20261007.c" user/elf-fs-runtime-lifetime.c
cp "$stage/fs-operation-syscall-20261007.inc" kernel/fs-operation-syscall.inc
printf 'RUNNING\n' >fs-runtime.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >fs-runtime.status' EXIT
bash build.sh >fs-runtime-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py >fs-runtime-bios.log 2>&1
python3 tests/test_fs_identity.py >fs-runtime-identity-bios.log 2>&1
python3 tests/test_runtime_asset.py >fs-runtime-asset-bios.log 2>&1
python3 tools/build_uefi.py >fs-runtime-uefi-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py --uefi >fs-runtime-uefi.log 2>&1
tail -n 1 fs-runtime-bios.log
tail -n 1 fs-runtime-uefi.log
