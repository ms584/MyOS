#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat descriptor-heap-queue-full.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/pmm-syscall-R4sQZZ/build/';then exit 1;fi
cp "$stage/fs-resident-accounting-20261008.h" include/fs.h
cp "$stage/ramfs-resident-accounting-20261008.c" fs/ramfs.c
cp "$stage/elf-clean-resident-accounting-20261008.c" kernel/elf.c
cp /home/xytron404/cobalt-closed-imports-xnVZeH/tests/cobalt_native_resource_inventory.py tests/cobalt_native_resource_inventory.py
printf 'RUNNING\n' >clean-resident-accounting-full.status
trap 'result=$?; bash build.sh >clean-resident-accounting-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >clean-resident-accounting-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >clean-resident-accounting-full.status' EXIT
bash build.sh >clean-resident-accounting-build.log 2>&1
python3 tools/build_uefi.py >clean-resident-accounting-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >clean-resident-accounting-normal.sha256
python3 "$stage/test_named_shm_lifetime_20261008.py" >clean-resident-accounting-lifetime-bios.log 2>&1
python3 "$stage/test_named_shm_lifetime_20261008.py" --uefi >clean-resident-accounting-lifetime-uefi.log 2>&1
python3 tests/test_shared_memory.py >clean-resident-accounting-shared-bios.log 2>&1
python3 tests/test_shared_memory.py --uefi >clean-resident-accounting-shared-uefi.log 2>&1
python3 tests/test_system.py >clean-resident-accounting-full.log 2>&1
bash build.sh >clean-resident-accounting-normal.log 2>&1
python3 tools/build_uefi.py >clean-resident-accounting-uefi-normal.log 2>&1
sha256sum -c clean-resident-accounting-normal.sha256
printf 'TERMINAL_EXIT=0\n' >clean-resident-accounting-full.status
trap - EXIT
