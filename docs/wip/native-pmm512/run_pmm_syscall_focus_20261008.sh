#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
candidate=$(mktemp -d /home/xytron404/pmm-syscall-XXXXXX)
cd "$candidate"
for item in AGENTS.md README.md build.sh build-phase22.sh test-boot.py test-exceptions.py test-timer.py test-vga.py assets audio boot cpu docs drivers fs gui include kernel memory net security shell tests third_party tools user vendor video; do cp -a "/home/xytron404/pmm-512m-01MfM9/$item" .; done
cp "$stage/elf-pmm-syscall-20261008.c" user/elf-pmm-syscall.c
cp "$stage/test_pmm_syscall_20261008.py" tests/test_pmm_syscall.py
printf '%s\n' "$candidate"
printf 'RUNNING\n' >pmm-syscall.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >pmm-syscall.status' EXIT
bash build.sh >pmm-syscall-old-build.log 2>&1
python3 tests/test_pmm_syscall.py --expect-old-failure >pmm-syscall-old-bios.log 2>&1
python3 tools/build_uefi.py >pmm-syscall-old-uefi-build.log 2>&1
python3 tests/test_pmm_syscall.py --uefi --expect-old-failure >pmm-syscall-old-uefi.log 2>&1
cp "$stage/syscall-pmm-owned-20261008.c" kernel/syscall.c
bash build.sh >pmm-syscall-build.log 2>&1
python3 tests/test_pmm_syscall.py >pmm-syscall-bios.log 2>&1
python3 tests/test_pmm_512m.py >pmm-syscall-memory-bios.log 2>&1
python3 tools/build_uefi.py >pmm-syscall-uefi-build.log 2>&1
python3 tests/test_pmm_syscall.py --uefi >pmm-syscall-uefi.log 2>&1
python3 tests/test_pmm_512m.py --uefi >pmm-syscall-memory-uefi.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >pmm-syscall-normal.sha256
tail -n 2 pmm-syscall-bios.log
tail -n 2 pmm-syscall-uefi.log
