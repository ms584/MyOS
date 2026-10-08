#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat cobalt-runtime-reservation.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing live VM mutation'; exit 1
fi
cp "$stage/platform-runtime-fd128-profile-20261008.c" user/platform-runtime.c
cp "$stage/elf-fs-runtime-reservation-20261008.c" user/elf-fs-runtime-lifetime.c
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
printf 'RUNNING\n' >cobalt-fd128-interactive.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-fd128-interactive.status' EXIT
python3 -m py_compile tests/test_cobalt_native_interactive_tls.py
bash build.sh >cobalt-fd128-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py >cobalt-fd128-runtime-bios.log 2>&1
cp build/system-tests/fs-runtime.log cobalt-fd128-runtime-bios-serial.txt
python3 tools/build_uefi.py >cobalt-fd128-uefi-build.log 2>&1
python3 tests/test_fs_runtime_lifetime.py --uefi >cobalt-fd128-runtime-uefi.log 2>&1
cp build/system-tests/fs-runtime-uefi.log cobalt-fd128-runtime-uefi-serial.txt
MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-fd128-interactive.log 2>&1
