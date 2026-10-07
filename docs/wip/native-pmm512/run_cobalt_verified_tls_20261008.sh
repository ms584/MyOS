#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
if pgrep -f '^python3 tests/test_cobalt_native_refresh_probe.py' >/dev/null; then
 echo 'Existing native refresh observation still running; refusing source/build mutation' >&2
 exit 1
fi
cp "$stage/process-cobalt-verified-tls-20261008.c" memory/process.c
cp "$stage/test_cobalt_native_verified_tls_20261008.py" tests/test_cobalt_native_verified_tls.py
printf 'RUNNING\n' >cobalt-verified-tls.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-verified-tls.status' EXIT
bash build.sh >cobalt-verified-tls-build.log 2>&1
python3 tests/test_cobalt_native_verified_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-verified-tls.log 2>&1
