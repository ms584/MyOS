#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-native-VzEALo
test "$(cat fault-timeout-audit.status)" = TERMINAL_EXIT=0
cp "$stage/test_system_fault_timeout_20261007.py" tests/test_system.py
trap 'result=$?; bash build.sh >fs-full-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >fs-full-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >fs-full.status' EXIT
bash "$stage/run_fs_full_regression_20261007.sh"
