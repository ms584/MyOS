#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/fs-native-VzEALo
test "$(cat fs-full.status)" = TERMINAL_EXIT=1
cp fs-full.log fs-full-fault-timeout-failure.log
cp fs-full.status fs-full-fault-timeout-failure.status
printf 'RUNNING\n' >fault-timeout-audit.status
trap 'result=$?; bash build.sh >fault-timeout-normal-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >fault-timeout-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >fault-timeout-audit.status' EXIT
python3 "$stage/probe_fault_timeout_20261007.py" >fault-timeout-audit.log 2>&1
tail -n 2 fault-timeout-audit.log
