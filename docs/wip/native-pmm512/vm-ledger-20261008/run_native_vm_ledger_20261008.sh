#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-atomic-pc.status)" = TERMINAL_EXIT=0
test ! -e include/vm-ledger.h
backup=$(mktemp -d build/vm-ledger-native-backup-XXXXXX)
cp memory/process.c "$backup/process.c"
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >vm-ledger-baseline-normal.sha256
printf 'RUNNING\n' >native-vm-ledger.status
trap 'result=$?; cp "$backup/process.c" memory/process.c; rm -- include/vm-ledger.h; bash build.sh >native-vm-ledger-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-vm-ledger-uefi-restore.log 2>&1 || result=1; sha256sum -c vm-ledger-baseline-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-vm-ledger.status' EXIT
cp "$stage/vm-ledger-20261008.h" include/vm-ledger.h
cp "$stage/process-vm-ledger-native-20261008.c" memory/process.c
bash build.sh >native-vm-ledger-build.log 2>&1
python3 tools/build_uefi.py >native-vm-ledger-uefi-build.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-vm-ledger-interactive.log 2>&1
