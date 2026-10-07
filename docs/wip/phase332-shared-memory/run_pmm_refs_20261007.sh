#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cd-QJYMwG
test "$(cat volume-audio-clean.status)" = 'TERMINAL_EXIT=0'
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >pmm-refs.status' EXIT
cp "$stage/memory-shared-20261007.c" memory/memory.c
cp "$stage/os-shared-20261007.h" include/os.h
cp "$stage/test_pmm_refs_20261007.py" tests/test_pmm_refs.py
PROCESS_TESTS=1 bash build.sh >pmm-refs-build.log 2>&1
python3 tests/test_pmm_refs.py >pmm-refs-native.log 2>&1
tail -n 8 pmm-refs-native.log
