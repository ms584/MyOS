#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat clean-resident-accounting-full.status)" = TERMINAL_EXIT=0
cp "$stage/vm-ledger-20261008.h" include/vm-ledger.h
cp "$stage/process-vm-ledger-clean-20261008.c" memory/process.c
printf 'RUNNING\n' >clean-vm-ledger-full.status
trap 'result=$?; bash build.sh >clean-vm-ledger-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >clean-vm-ledger-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >clean-vm-ledger-full.status' EXIT
bash build.sh >clean-vm-ledger-build.log 2>&1
python3 tools/build_uefi.py >clean-vm-ledger-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >clean-vm-ledger-normal.sha256
for test in test_vm_hint.py test_shared_memory.py; do
 python3 "tests/$test" >"clean-vm-ledger-$test-bios.log" 2>&1
 python3 "tests/$test" --uefi >"clean-vm-ledger-$test-uefi.log" 2>&1
done
python3 "$stage/test_named_shm_lifetime_20261008.py" >clean-vm-ledger-lifetime-bios.log 2>&1
python3 "$stage/test_named_shm_lifetime_20261008.py" --uefi >clean-vm-ledger-lifetime-uefi.log 2>&1
python3 tests/test_system.py >clean-vm-ledger-full.log 2>&1
bash build.sh >clean-vm-ledger-normal.log 2>&1
python3 tools/build_uefi.py >clean-vm-ledger-uefi-normal.log 2>&1
sha256sum -c clean-vm-ledger-normal.sha256
printf 'TERMINAL_EXIT=0\n' >clean-vm-ledger-full.status
trap - EXIT
