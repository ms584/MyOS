#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/native-integrated-XZBHuo
test "$(cat integrated-regression.status)" = TERMINAL_EXIT=1
grep -F 'fatal error: ../third_party/compiler-rt/emutls.c: No such file or directory' integrated-full-system.log
sha256sum -c integrated-normal.sha256
cp integrated-regression.status integrated-regression-first.status
# The first run stopped in host fixture compilation, before its emutls guest.
# Restore the exact pinned test-only inputs, never substitute system compiler-rt.
cp -a /home/xytron404/myos/third_party .
cp /home/xytron404/myos/build/integrated-source-audit-20261008.json /home/xytron404/myos/build/integrated-source-audit-before-dependency-20261008.json
python3 "$stage/audit_integrated_native_20261008.py"
python3 - <<'PY'
from pathlib import Path
import json
root=Path('/home/xytron404/myos/build')
old=json.loads((root/'integrated-source-audit-before-dependency-20261008.json').read_text())
new=json.loads((root/'integrated-source-audit-20261008.json').read_text())
assert old['changes']==new['changes'], 'Guest or test source drifted since native/first regression'
print('PASS prior sixteen changed-file hashes unchanged; pinned dependency restored')
PY
gcc -m64 -std=c11 -Os -fPIC -ffreestanding -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -Wall -Wextra -Werror -Itests/emutls-abi -c user/elf-platform-emutls.c -o build/system-tests/elf-dynamic/elf-platform-emutls.o
printf 'RUNNING\n' >integrated-regression.status
trap 'result=$?; bash build.sh >integrated-regression-retry-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >integrated-regression-retry-uefi-restore.log 2>&1 || result=1; sha256sum -c integrated-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >integrated-regression.status' EXIT
# Reuse the already terminal-passed ten focused BIOS/UEFI runs on identical
# source. Run the complete default system suite again, not a suffix/subset.
python3 tests/test_system.py >integrated-full-system-retry.log 2>&1
