#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-egl-timing.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/';then exit 1;fi
cp kernel/syscall.c build/syscall-before-errno-count.c
cp "$stage/syscall-errno-count-20261008.c" kernel/syscall.c
printf 'RUNNING\n' >native-errno-count.status
trap 'result=$?; cp build/syscall-before-errno-count.c kernel/syscall.c; bash build.sh >native-errno-count-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >native-errno-count-uefi-restore.log 2>&1 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >native-errno-count.status' EXIT
bash build.sh >native-errno-count-build.log 2>&1
provider=$(cat egl-timing-provider-path.txt)
MYOS_TLS_PRESENT_TIMING=1 MYOS_RENDERER_PROVIDER_PATH="$provider" MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=1 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-errno-count-interactive.log 2>&1
