#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-tls-relaunch.status)" = TERMINAL_EXIT=0
cp "$stage/test_cobalt_native_verified_tls_20261008.py" tests/test_cobalt_native_verified_tls.py
cp "$stage/cobalt_native_resource_inventory_20261008.py" tests/cobalt_native_resource_inventory.py
python3 -m py_compile tests/test_cobalt_native_verified_tls.py tests/cobalt_native_resource_inventory.py
printf 'RUNNING\n' >cobalt-tls-resource-profile.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-tls-resource-profile.status' EXIT
MYOS_TLS_RESOURCE_DIAGNOSTIC=1 python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-tls-resource-profile.log 2>&1
