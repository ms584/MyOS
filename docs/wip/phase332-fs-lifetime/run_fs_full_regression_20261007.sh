#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/fs-native-VzEALo
test "$(cat fs-runtime.status)" = TERMINAL_EXIT=0
printf 'RUNNING\n' >fs-full.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >fs-full.status' EXIT
# Nondiagnostic source tree is frozen for this whole sequence.
PROCESS_TESTS=1 bash build.sh >fs-full-build.log 2>&1
test "$(nm -g build/platform-runtime.elf | grep -c MyOSMissingImport || true)" = 0
python3 tests/test_system.py >fs-full.log 2>&1
bash build.sh >fs-full-normal-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py >fs-full-runtime-bios.log 2>&1
python3 tests/test_fs_identity.py >fs-full-identity-bios.log 2>&1
python3 tests/test_runtime_asset.py >fs-full-assets-bios.log 2>&1
python3 tests/test_shared_memory.py >fs-full-shared-bios.log 2>&1
python3 tests/test_cpu_clock.py >fs-full-cpu-clock-bios.log 2>&1
python3 tools/build_uefi.py >fs-full-uefi-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py --uefi >fs-full-runtime-uefi.log 2>&1
python3 tests/test_fs_identity.py --uefi >fs-full-identity-uefi.log 2>&1
python3 tests/test_shared_memory.py --uefi >fs-full-shared-uefi.log 2>&1
python3 tests/test_cpu_clock.py --uefi >fs-full-cpu-clock-uefi.log 2>&1
python3 tests/test_starboard_launcher.py --uefi >fs-full-launcher-uefi.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >fs-full-normal.sha256
tail -n 4 fs-full.log
