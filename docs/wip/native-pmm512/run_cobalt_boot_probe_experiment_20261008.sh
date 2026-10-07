#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-refresh-probe.status)" = TERMINAL_EXIT=0
cp "$stage/scheduler-boot-probes-20261008.c" cpu/scheduler.c
cp "$stage/kernel-boot-probes-20261008.c" kernel/kernel.c
cp "$stage/process-test-boot-probes-20261008.c" user/process-test.c
cp "$stage/os-boot-probes-20261008.h" include/os.h
cp "$stage/test_boot_probe_cleanup_20261008.py" tests/test_boot_probe_cleanup.py
printf 'RUNNING\n' >cobalt-boot-probe-experiment.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >cobalt-boot-probe-experiment.status' EXIT
bash build.sh >cobalt-boot-probe-build.log 2>&1
python3 tools/build_uefi.py >cobalt-boot-probe-uefi-build.log 2>&1
python3 tests/test_boot_probe_cleanup.py >cobalt-boot-probe-focus.log 2>&1
python3 tests/test_cpu_clock.py >cobalt-boot-probe-cpu-bios.log 2>&1
python3 tests/test_cpu_clock.py --uefi >cobalt-boot-probe-cpu-uefi.log 2>&1
python3 tests/test_cobalt_native_refresh_probe.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-boot-probe-native.log 2>&1
