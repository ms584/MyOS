#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/player-unavailable-bI04LM
main=/home/xytron404/myos
test "$(cat "$clean/raw-clock.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/image-protect-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools memory cpu; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$clean/build.sh" "$probe/build.sh"
cp "$stage/process-image-protect-20261007.c" "$probe/memory/process.c"
cp "$stage/elf-image-protect-20261007.c" "$probe/user/elf-image-protect.c"
cp "$stage/elf-system-path-window-args-20261007.c" "$probe/user/elf-system-path.c"
cp "$stage/test_image_protect_20261007.py" "$probe/tests/test_image_protect.py"
cd "$probe";mkdir -p build
ln -s "$main/build/engine-evaluation" build/engine-evaluation
printf 'IMAGE_PROTECT_TREE=%s\n' "$probe"
printf 'RUNNING\n' >image-protect.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >image-protect.status' EXIT
bash build.sh >image-protect-build.log 2>&1
python3 tests/test_image_protect.py >image-protect-bios.log 2>&1
python3 tests/test_elf_dynamic.py >image-protect-elf-bios.log 2>&1
python3 tools/build_uefi.py >image-protect-uefi-build.log 2>&1
python3 tests/test_image_protect.py --uefi >image-protect-uefi.log 2>&1
tail -n 1 image-protect-bios.log
tail -n 1 image-protect-uefi.log
