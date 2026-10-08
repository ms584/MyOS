#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-vm-ledger-gs.status)" = TERMINAL_EXIT=0
test ! -e include/vm-ledger.h
test -f tests/test_named_shm_lifetime.py
test -f user/elf-named-shm-lifetime.c
backup=$(mktemp -d build/batched-prune-backup-XXXXXX)
cp memory/process.c "$backup/process.c"
cp user/platform-runtime.c "$backup/runtime.c"
cp cpu/scheduler.c "$backup/scheduler.c"
sha256sum build/kernel.bin build/platform-runtime.bin build/platform-runtime.mrt build/myos-current.img build/myos-uefi.img >batched-prune-baseline-normal.sha256
printf 'RUNNING\n' >native-batched-prune.status
trap 'result=$?; cp "$backup/process.c" memory/process.c; cp "$backup/runtime.c" user/platform-runtime.c; cp "$backup/scheduler.c" cpu/scheduler.c; rm -- include/vm-ledger.h; bash build.sh >native-batched-prune-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-batched-prune-uefi-restore.log 2>&1 || result=1; sha256sum -c batched-prune-baseline-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-batched-prune.status' EXIT
cp "$stage/vm-ledger-20261008.h" include/vm-ledger.h
cp "$stage/process-vm-batched-prune-native-20261008.c" memory/process.c
cp "$stage/runtime-gs-errno-20261008.c" user/platform-runtime.c
cp "$stage/scheduler-gs-errno-20261008.c" cpu/scheduler.c
bash build.sh >native-batched-prune-build.log 2>&1
python3 tools/build_uefi.py >native-batched-prune-uefi-build.log 2>&1
python3 tests/test_shared_memory.py >native-batched-prune-shared-bios.log 2>&1
python3 tests/test_shared_memory.py --uefi >native-batched-prune-shared-uefi.log 2>&1
python3 tests/test_named_shm_lifetime.py >native-batched-prune-lifetime-bios.log 2>&1
python3 tests/test_named_shm_lifetime.py --uefi >native-batched-prune-lifetime-uefi.log 2>&1
PYTHONPATH="$PWD/tests" MYOS_TLS_VISUAL_LATENCY=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 "$stage/test_cobalt_native_interactive_tls_20261008.py" /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-batched-prune-interactive.log 2>&1
