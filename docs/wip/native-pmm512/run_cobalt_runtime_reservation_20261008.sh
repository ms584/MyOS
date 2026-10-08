#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat runtime-reservation-concurrent.status)" = TERMINAL_EXIT=0
printf 'RUNNING\n' >cobalt-runtime-reservation.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-runtime-reservation.status' EXIT
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-runtime-reservation.log 2>&1
