#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/player-unavailable-bI04LM
test "$(cat player-unavailable.status)" = TERMINAL_EXIT=0
cp "$stage/build-fs-lifetime-20261007.sh" build.sh
cp "$stage/exports-fs-lifetime-20261007.py" tools/make_runtime_exports.py
cp "$stage/platform-fs-unavailable-20261007.c" user/platform-fs-unavailable.c
cp "$stage/elf-fs-unavailable-20261007.c" user/elf-fs-unavailable.c
cp "$stage/test_fs_unavailable_20261007.py" tests/test_fs_unavailable.py
printf 'RUNNING\n' >import-closure.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >import-closure.status' EXIT
bash build.sh >import-closure-build.log 2>&1
python3 tests/test_fs_unavailable.py >import-closure-bios.log 2>&1
python3 tests/test_player_unavailable.py >import-closure-player-bios.log 2>&1
python3 tools/build_uefi.py >import-closure-uefi-build.log 2>&1
python3 tests/test_fs_unavailable.py --uefi >import-closure-uefi.log 2>&1
python3 tests/test_player_unavailable.py --uefi >import-closure-player-uefi.log 2>&1
python3 "$stage/census_clean_imports_20261007.py" /home/xytron404/player-unavailable-bI04LM >import-closure-census.log
tail -n 1 import-closure-bios.log
tail -n 1 import-closure-uefi.log
cat import-closure-census.log
