#!/usr/bin/env bash
set -euo pipefail
clean=/home/xytron404/image-protect-TSMFT6
grep -q '^PASS native ELF mprotect/' "$clean/image-protect-bios.log"
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-raw-clock.status)" = TERMINAL_EXIT=0
cp "$clean/memory/process.c" memory/process.c
printf 'RUNNING\n' >cobalt-image-protect.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-image-protect.status' EXIT
bash build.sh >cobalt-image-protect-build.log 2>&1
if nm build/platform-runtime.elf | grep -q MyOSMissing; then exit 1; fi
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-image-protect.log 2>&1
tail -n 35 cobalt-image-protect.log
