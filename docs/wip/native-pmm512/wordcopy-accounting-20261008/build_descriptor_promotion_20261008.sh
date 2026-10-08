#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
python3 "$stage/promote_descriptor_heap_queue_20261008.py"
printf 'RUNNING\n' >descriptor-promotion.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >descriptor-promotion.status' EXIT
bash build.sh >descriptor-promotion-build.log 2>&1
python3 tools/build_uefi.py >descriptor-promotion-uefi-build.log 2>&1
cp /home/xytron404/pmm-syscall-R4sQZZ/descriptor-heap-queue-normal.sha256 build/descriptor-promotion-normal.sha256
sha256sum -c build/descriptor-promotion-normal.sha256
git diff --check
