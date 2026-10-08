#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat native-resident-accounting.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then exit 1; fi
# Retain failed log: stale private fixture expected the superseded 1MiB ceiling.
# Use current production 8MiB fixture and its matching driver, not relaxed checks.
cp /home/xytron404/myos/user/elf-shared-memory.c user/elf-shared-memory.c
cp /home/xytron404/myos/tests/test_shared_memory.py tests/test_shared_memory.py
printf 'RUNNING\n' >native-resident-accounting-resume.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >native-resident-accounting-resume.status' EXIT
python3 tests/test_shared_memory.py >native-resident-accounting-shared-current-bios.log 2>&1
python3 tests/test_shared_memory.py --uefi >native-resident-accounting-shared-current-uefi.log 2>&1
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=3 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >native-resident-accounting-interactive.log 2>&1
