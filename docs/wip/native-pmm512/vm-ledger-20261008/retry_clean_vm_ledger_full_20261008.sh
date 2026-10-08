#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat clean-vm-ledger-full.status)" = TERMINAL_EXIT=1
cp clean-vm-ledger-full.log clean-vm-ledger-first-full-failure.log
cp "$stage/test-desktop-frame-fence-20261008.py" tests/test_desktop.py
printf 'RUNNING\n' >clean-vm-ledger-retry.status
trap 'result=$?; bash build.sh >clean-vm-ledger-retry-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >clean-vm-ledger-retry-uefi-restore.log 2>&1 || result=1; sha256sum -c clean-vm-ledger-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >clean-vm-ledger-retry.status' EXIT
python3 tests/test_system.py --suite desktop >clean-vm-ledger-desktop-retry.log 2>&1
python3 tests/test_system.py >clean-vm-ledger-full-retry.log 2>&1
