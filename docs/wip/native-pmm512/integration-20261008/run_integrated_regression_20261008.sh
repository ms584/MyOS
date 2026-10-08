#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/native-integrated-XZBHuo
test "$(cat integrated-native.status)" = TERMINAL_EXIT=0
sha256sum -c integrated-normal.sha256
# Pinned compile-time headers used by existing tests, not guest libraries.
mkdir -p build/engine-evaluation/contracts
cp -a /home/xytron404/myos/build/engine-evaluation/contracts/. build/engine-evaluation/contracts/
printf 'RUNNING\n' >integrated-regression.status
trap 'result=$?; bash build.sh >integrated-regression-restore.log 2>&1 || result=1; python3 tools/build_uefi.py >integrated-regression-uefi-restore.log 2>&1 || result=1; sha256sum -c integrated-normal.sha256 || result=1; printf "TERMINAL_EXIT=%s\n" "$result" >integrated-regression.status' EXIT
for mode in bios uefi; do
 options=();if test "$mode" = uefi; then options=(--uefi);fi
 python3 "$stage/test_gs_errno_20261008.py" "${options[@]}" >"integrated-gs-errno-$mode.log" 2>&1
 python3 "$stage/test_gs_custom_fs_20261008.py" "${options[@]}" >"integrated-gs-fs-$mode.log" 2>&1
 python3 tests/test_shared_memory.py "${options[@]}" >"integrated-shared-$mode.log" 2>&1
 python3 tests/test_named_shm_lifetime.py "${options[@]}" >"integrated-lifetime-$mode.log" 2>&1
 python3 tests/test_starboard_system.py --launcher "${options[@]}" >"integrated-path-$mode.log" 2>&1
done
python3 tests/test_system.py >integrated-full-system.log 2>&1
