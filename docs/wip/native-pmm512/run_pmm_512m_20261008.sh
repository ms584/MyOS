#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
candidate=$(mktemp -d /home/xytron404/pmm-512m-XXXXXX)
cd "$candidate"
for item in AGENTS.md README.md build.sh build-phase22.sh test-boot.py test-exceptions.py test-timer.py test-vga.py assets audio boot cpu docs drivers fs gui include kernel memory net security shell tests third_party tools user vendor video; do
 cp -a "/home/xytron404/shm-8m-4hJG2I/$item" .
done
cp "$stage/memory-512m-20261008.c" memory/memory.c
cp "$stage/test_pmm_512m_20261008.py" tests/test_pmm_512m.py
printf '%s\n' "$candidate"
printf 'RUNNING\n' >pmm512.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >pmm512.status' EXIT
PROCESS_TESTS=1 bash build.sh >pmm512-build.log 2>&1
python3 tests/test_pmm_512m.py >pmm512-bios.log 2>&1
python3 tools/build_uefi.py >pmm512-uefi-build.log 2>&1
python3 tests/test_pmm_512m.py --uefi >pmm512-uefi.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >pmm512-normal.sha256
tail -n 8 pmm512-bios.log
tail -n 8 pmm512-uefi.log
