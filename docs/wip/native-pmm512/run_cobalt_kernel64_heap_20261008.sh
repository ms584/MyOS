#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
candidate=/home/xytron404/pmm-syscall-R4sQZZ
test "$(cat cobalt-cpu-accounting.status)" = TERMINAL_EXIT=0
test "$(cat cobalt-fd128-interactive.status)" = TERMINAL_EXIT=0
for firmware in bios uefi; do
 for scope in fs_capacity fs_runtime_lifetime statvfs shared_memory server; do
  grep -q '^PASS ' "$candidate/descriptor-heap-$scope-$firmware.log"
 done
done
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing live VM mutation'; exit 1
fi
diff -u /home/xytron404/myos/include/process.h include/process.h
cp "$stage/fs-shm-pool-handles64-20261008.h" include/fs.h
cp "$stage/process-fs-table-heap-20261008.h" include/process.h
cp "$stage/process-native-fs-table-heap-20261008.c" memory/process.c
cp "$stage/elf-fs-capacity-fd128-20261008.c" user/elf-fs-capacity.c
cp "$stage/test_fs_capacity_fd128_20261008.py" tests/test_fs_capacity.py
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
printf 'RUNNING\n' >cobalt-kernel64-heap.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-kernel64-heap.status' EXIT
bash build.sh >cobalt-kernel64-heap-build.log 2>&1
python3 tests/test_fs_capacity.py >cobalt-kernel64-heap-capacity-bios.log 2>&1
python3 tools/build_uefi.py >cobalt-kernel64-heap-uefi-build.log 2>&1
python3 tests/test_fs_capacity.py --uefi >cobalt-kernel64-heap-capacity-uefi.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-kernel64-heap-interactive.log 2>&1
