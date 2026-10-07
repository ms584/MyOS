#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-native-http-probe.status)" = TERMINAL_EXIT=0
strings build/engine-evaluation/libcobalt.so | grep -Fx https-enforcement
cp "$stage/process-cobalt-http-diagnostic-20261008.c" memory/process.c
cp "$stage/test_cobalt_native_http_probe_20261008.py" tests/test_cobalt_native_http_policy_probe.py
printf 'RUNNING\n' >cobalt-http-policy-probe.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-http-policy-probe.status' EXIT
bash build.sh >cobalt-http-policy-probe-build.log 2>&1
python3 tests/test_cobalt_native_http_policy_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-http-policy-probe.log 2>&1
tail -n 5 cobalt-http-policy-probe.log
