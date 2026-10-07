#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/cpu-clock-HEdtNZ
main=/home/xytron404/myos
test "$(cat "$clean/window-args.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/platform-fonts-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools memory cpu; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$stage/build-fs-lifetime-20261007.sh" "$probe/build.sh"
cp "$stage/syscall-platform-fonts-20261007.c" "$probe/kernel/syscall.c"
cp "$stage/elf-platform-fonts-20261007.c" "$probe/user/elf-platform-fonts.c"
cp "$stage/test_platform_fonts_20261007.py" "$probe/tests/test_platform_fonts.py"
cd "$probe";mkdir -p build
printf 'PLATFORM_FONTS_TREE=%s\n' "$probe"
printf 'RUNNING\n' >platform-fonts.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >platform-fonts.status' EXIT
python3 "$stage/prepare_platform_fonts_20261007.py" "$probe" >platform-fonts-assets.log 2>&1
bash build.sh >platform-fonts-build.log 2>&1
python3 tests/test_platform_fonts.py >platform-fonts-bios.log 2>&1
python3 tools/build_uefi.py >platform-fonts-uefi-build.log 2>&1
python3 tests/test_platform_fonts.py --uefi >platform-fonts-uefi.log 2>&1
tail -n 1 platform-fonts-bios.log
tail -n 1 platform-fonts-uefi.log
