#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat cobalt-kernel64-heap.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing live VM mutation'; exit 1
fi
cp "$stage/elf-native-page-diagnostic-20261008.c" kernel/elf.c
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
printf 'RUNNING\n' >cobalt-close-diagnostic.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-close-diagnostic.status' EXIT
bash build.sh >cobalt-close-diagnostic-build.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=1 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-close-diagnostic.log 2>&1
