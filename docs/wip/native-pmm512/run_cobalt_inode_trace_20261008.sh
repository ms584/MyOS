#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-tls-inode.status)" = TERMINAL_EXIT=0
cp "$stage/ramfs-inode-trace-20261008.c" fs/ramfs.c
printf 'RUNNING\n' >cobalt-inode-trace.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-inode-trace.status' EXIT
bash build.sh >cobalt-inode-trace-build.log 2>&1
MYOS_TLS_RESOURCE_DIAGNOSTIC=1 python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-inode-trace.log 2>&1
