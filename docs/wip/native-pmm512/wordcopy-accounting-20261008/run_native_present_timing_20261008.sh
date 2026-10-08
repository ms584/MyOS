#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-resident-accounting-resume.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/';then exit 1;fi
cp kernel/syscall.c build/syscall-before-present-timing.c
cp "$stage/syscall-present-timing-20261008.c" kernel/syscall.c
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
printf 'RUNNING\n' >native-present-timing.status
trap 'result=$?; cp build/syscall-before-present-timing.c kernel/syscall.c; bash build.sh >native-present-timing-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-present-timing-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-present-timing.status' EXIT
bash build.sh >native-present-timing-build.log 2>&1
MYOS_TLS_PRESENT_TIMING=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=1 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-present-timing-interactive.log 2>&1
