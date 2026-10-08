#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-close-diagnostic.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing concurrent native VM'; exit 1
fi
printf 'RUNNING\n' >cobalt-close-repeat.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-close-repeat.status' EXIT
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=3 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-close-repeat.log 2>&1
