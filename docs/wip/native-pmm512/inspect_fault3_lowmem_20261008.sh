#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat descriptor-heap-full.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/pmm-syscall-R4sQZZ/build/'; then
 echo 'Refusing live candidate mutation'; exit 1
fi
cp descriptor-heap-full.log descriptor-heap-fault3-full-failure.log
cp descriptor-heap-full.status descriptor-heap-fault3-full-failure.status
trap 'bash build.sh >fault3-inspect-normal.log 2>&1; python3 tools/build_uefi.py >fault3-inspect-uefi-normal.log 2>&1' EXIT
if TEST_CASE=3 PROCESS_TESTS=1 bash build.sh >fault3-inspect-build.log 2>&1; then
 echo 'Fault3 unexpectedly built; investigate previous failure'; exit 1
fi
cp build/kernel.map fault3-inspect-kernel.map
grep -n -e '__bss_end' -e '__bss_start' -e '^.bss' -e '^.text' -e '^.rodata' -e '^.data' fault3-inspect-kernel.map
