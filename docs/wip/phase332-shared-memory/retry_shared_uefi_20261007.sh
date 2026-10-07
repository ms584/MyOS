#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cd-QJYMwG
test "$(cat shared-memory.status)" = 'TERMINAL_EXIT=1'
cp shared-memory-uefi.log shared-memory-uefi-16mb-prekernel-failure.log
cp "$stage/test_shared_memory_20261007.py" tests/test_shared_memory.py
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >shared-memory-uefi-retry.status' EXIT
# Binary/source unchanged. Retry supported OVMF RAM sizes only.
python3 tests/test_shared_memory.py --uefi >shared-memory-uefi-retry.log 2>&1
tail -n 4 shared-memory-uefi-retry.log
