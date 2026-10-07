#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/cpu-clock-HEdtNZ
test "$(cat cpu-clock.status)" = TERMINAL_EXIT=0
cp "$stage/process-window-args-20261007.c" memory/process.c
cp "$stage/syscall-window-args-20261007.c" kernel/syscall.c
cp "$stage/elf-starboard-window-args-20261007.c" user/elf-starboard-launcher.c
mkdir -p build
test -L build/engine-evaluation || ln -s /home/xytron404/myos/build/engine-evaluation build/engine-evaluation
if test -f window-args-bios.log; then
 cp window-args-bios.log window-args-bios-before-executable-path-fix.log
 cp window-args-uefi.log window-args-uefi-before-executable-path-fix.log
fi
printf 'RUNNING\n' >window-args.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >window-args.status' EXIT
bash build.sh >window-args-build.log 2>&1
python3 tests/test_starboard_launcher.py >window-args-bios.log 2>&1
python3 tools/build_uefi.py >window-args-uefi-build.log 2>&1
python3 tests/test_starboard_launcher.py --uefi >window-args-uefi.log 2>&1
tail -n 2 window-args-bios.log
tail -n 2 window-args-uefi.log
