#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-tls-verbose.status)" = TERMINAL_EXIT=0
if pgrep -f '^qemu-system-.*cobalt-closed-imports-xnVZeH' >/dev/null; then
    echo 'Existing candidate VM must finish before fixture edit' >&2
    exit 1
fi
cp "$stage/test_cobalt_native_verified_tls_20261008.py" tests/test_cobalt_native_verified_tls.py
python3 -m py_compile tests/test_cobalt_native_verified_tls.py
printf 'RUNNING\n' >cobalt-tls-csp.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-tls-csp.status' EXIT
python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-tls-csp.log 2>&1
