#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat descriptor-heap-full.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/pmm-syscall-R4sQZZ/build/'; then
 echo 'Refusing live candidate mutation'; exit 1
fi
cp "$stage/sb16-queue-heap-20261008.c" drivers/sb16.c
printf 'RUNNING\n' >descriptor-heap-queue-full.status
trap 'result=$?; bash build.sh >descriptor-heap-queue-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >descriptor-heap-queue-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >descriptor-heap-queue-full.status' EXIT
# Reproduce every fault-variant build before investing in the entire suite.
for case in 0 1 2 3 4 5 6; do
 TEST_CASE="$case" PROCESS_TESTS=1 bash build.sh >"descriptor-heap-queue-fault-$case.log" 2>&1
 cp build/kernel.map "descriptor-heap-queue-fault-$case.map"
done
bash build.sh >descriptor-heap-queue-build.log 2>&1
python3 tools/build_uefi.py >descriptor-heap-queue-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >descriptor-heap-queue-normal.sha256
python3 tests/test_pcm.py >descriptor-heap-queue-pcm-bios.log 2>&1
python3 tests/test_pcm.py --uefi >descriptor-heap-queue-pcm-uefi.log 2>&1
python3 tests/test_system.py >descriptor-heap-queue-full.log 2>&1
bash build.sh >descriptor-heap-queue-normal.log 2>&1
python3 tools/build_uefi.py >descriptor-heap-queue-uefi-normal.log 2>&1
sha256sum -c descriptor-heap-queue-normal.sha256
printf 'TERMINAL_EXIT=0\n' >descriptor-heap-queue-full.status
trap - EXIT
