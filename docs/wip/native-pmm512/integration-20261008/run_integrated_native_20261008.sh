#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/native-integrated-XZBHuo
native=/home/xytron404/cobalt-closed-imports-xnVZeH
mkdir -p build/engine-evaluation build/renderer-provider-evaluation/provider-84sokh6g
cp "$native/build/engine-evaluation/libcobalt.so" build/engine-evaluation/
cp "$native/build/renderer-provider-evaluation/provider-84sokh6g/provider.so" build/renderer-provider-evaluation/provider-84sokh6g/
cp "$native/build/renderer-provider-evaluation/provider-84sokh6g/report.json" build/renderer-provider-evaluation/provider-84sokh6g/
cp "$stage/test_cobalt_native_acceptance_20261008.py" tests/test_cobalt_native_acceptance.py
cp "$stage/cobalt_native_resource_inventory_20261008.py" tests/cobalt_native_resource_inventory.py
cp "$stage/cobalt_pmm_observer_20261008.py" tests/cobalt_pmm_observer.py
printf 'RUNNING\n' >integrated-native.status
trap 'result=$?; sha256sum -c integrated-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >integrated-native.status' EXIT
sha256sum -c integrated-normal.sha256
env MYOS_TLS_VISUAL_LATENCY=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_RAM_SAMPLING=1 MYOS_TLS_LAUNCHES=2 \
 python3 "$stage/run_existing_kvm_test_20261008.py" tests/test_cobalt_native_acceptance.py \
 /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n \
 /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >integrated-native.log 2>&1
