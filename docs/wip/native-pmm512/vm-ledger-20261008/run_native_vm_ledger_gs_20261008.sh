#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-vm-ledger.status)" = TERMINAL_EXIT=0
test ! -e include/vm-ledger.h
backup=$(mktemp -d build/vm-ledger-gs-backup-XXXXXX)
cp memory/process.c "$backup/process.c"
cp user/platform-runtime.c "$backup/runtime.c"
cp cpu/scheduler.c "$backup/scheduler.c"
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >vm-ledger-gs-baseline-normal.sha256
printf 'RUNNING\n' >native-vm-ledger-gs.status
trap 'result=$?; cp "$backup/process.c" memory/process.c; cp "$backup/runtime.c" user/platform-runtime.c; cp "$backup/scheduler.c" cpu/scheduler.c; rm -- include/vm-ledger.h; bash build.sh >native-vm-ledger-gs-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-vm-ledger-gs-uefi-restore.log 2>&1 || result=1; sha256sum -c vm-ledger-gs-baseline-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-vm-ledger-gs.status' EXIT
cp "$stage/vm-ledger-20261008.h" include/vm-ledger.h
cp "$stage/process-vm-ledger-gs-native-20261008.c" memory/process.c
cp "$stage/runtime-gs-errno-20261008.c" user/platform-runtime.c
cp "$stage/scheduler-gs-errno-20261008.c" cpu/scheduler.c
bash build.sh >native-vm-ledger-gs-build.log 2>&1
python3 tools/build_uefi.py >native-vm-ledger-gs-uefi-build.log 2>&1
python3 "$stage/test_gs_errno_20261008.py" >native-vm-ledger-gs-oracle-bios.log 2>&1
python3 "$stage/test_gs_errno_20261008.py" --uefi >native-vm-ledger-gs-oracle-uefi.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-vm-ledger-gs-interactive.log 2>&1
