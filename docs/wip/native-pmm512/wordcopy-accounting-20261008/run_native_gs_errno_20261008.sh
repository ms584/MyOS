#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-errno-count.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/';then exit 1;fi
backup=$(mktemp -d build/gs-errno-backup-XXXXXX)
cp user/platform-runtime.c "$backup/runtime.c"
cp memory/process.c "$backup/process.c"
cp cpu/scheduler.c "$backup/scheduler.c"
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >gs-errno-baseline-normal.sha256
cp "$stage/runtime-gs-errno-20261008.c" user/platform-runtime.c
cp "$stage/process-gs-errno-20261008.c" memory/process.c
cp "$stage/scheduler-gs-errno-20261008.c" cpu/scheduler.c
sha256sum user/platform-runtime.c memory/process.c cpu/scheduler.c >gs-errno-source.sha256
printf 'RUNNING\n' >native-gs-errno.status
trap 'result=$?; cp "$backup/runtime.c" user/platform-runtime.c; cp "$backup/process.c" memory/process.c; cp "$backup/scheduler.c" cpu/scheduler.c; bash build.sh >native-gs-errno-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-gs-errno-uefi-restore.log 2>&1 || result=1; sha256sum -c gs-errno-baseline-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-gs-errno.status' EXIT
bash build.sh >native-gs-errno-build.log 2>&1
python3 tools/build_uefi.py >native-gs-errno-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >gs-errno-trial-normal.sha256
python3 "$stage/test_gs_errno_20261008.py" >native-gs-errno-oracle-bios.log 2>&1
python3 "$stage/test_gs_errno_20261008.py" --uefi >native-gs-errno-oracle-uefi.log 2>&1
python3 tests/test_thread_services.py >native-gs-errno-thread-services.log 2>&1
MYOS_TLS_PRESENT_TIMING=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-gs-errno-interactive.log 2>&1
