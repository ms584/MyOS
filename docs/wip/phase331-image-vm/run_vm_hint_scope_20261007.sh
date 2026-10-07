#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/image-protect-TSMFT6
main=/home/xytron404/myos
test "$(cat "$clean/image-protect.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/vm-hint-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools memory cpu; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$clean/build.sh" "$probe/build.sh"
cp "$stage/process-vm-hint-20261007.c" "$probe/memory/process.c"
cp "$stage/process-vm-hint-20261007.h" "$probe/include/process.h"
cp "$stage/syscall-vm-hint-20261007.c" "$probe/kernel/syscall.c"
cp "$stage/syscall-vm-hint-20261007.h" "$probe/include/syscall.h"
cp "$stage/runtime-vm-hint-20261007.c" "$probe/user/platform-runtime.c"
cp "$stage/elf-vm-hint-20261007.c" "$probe/user/elf-vm-hint.c"
cp "$stage/test_vm_hint_20261007.py" "$probe/tests/test_vm_hint.py"
cp "$stage/test_image_protect_20261007.py" "$probe/tests/test_image_protect.py"
cd "$probe";mkdir -p build
ln -s "$main/build/engine-evaluation" build/engine-evaluation
printf 'VM_HINT_TREE=%s\n' "$probe"
printf 'RUNNING\n' >vm-hint.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >vm-hint.status' EXIT
bash build.sh >vm-hint-build.log 2>&1
python3 tests/test_vm_hint.py >vm-hint-bios.log 2>&1
python3 tests/test_image_protect.py >vm-hint-image-bios.log 2>&1
python3 tools/build_uefi.py >vm-hint-uefi-build.log 2>&1
python3 tests/test_vm_hint.py --uefi >vm-hint-uefi.log 2>&1
python3 tests/test_image_protect.py --uefi >vm-hint-image-uefi.log 2>&1
tail -n 1 vm-hint-bios.log
tail -n 1 vm-hint-uefi.log
