#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-inode-trace.status)" = TERMINAL_EXIT=0
cp "$stage/fs-shm-pool-20261008.h" include/fs.h
cp "$stage/ramfs-shm-pool-20261008.c" fs/ramfs.c
cp "$stage/snapshot-shm-pool-20261008.inc" fs/snapshot-impl.inc
cp "$stage/fs-test-shm-pool-20261008.c" fs/fs-test.c
cp "$stage/test_cobalt_native_verified_tls_20261008.py" tests/test_cobalt_native_verified_tls.py
cp "$stage/cobalt_native_resource_inventory_20261008.py" tests/cobalt_native_resource_inventory.py
python3 -m py_compile tests/test_cobalt_native_verified_tls.py tests/cobalt_native_resource_inventory.py
printf 'RUNNING\n' >cobalt-shm-pool.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-shm-pool.status' EXIT
bash build.sh >cobalt-shm-pool-build.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-shm-pool.log 2>&1
