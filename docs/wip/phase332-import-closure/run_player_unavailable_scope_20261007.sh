#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/platform-fonts-01W7Ri
main=/home/xytron404/myos
test "$(cat "$clean/data-limit.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/player-unavailable-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools memory cpu; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$stage/build-fs-lifetime-20261007.sh" "$probe/build.sh"
cp "$stage/exports-fs-lifetime-20261007.py" "$probe/tools/make_runtime_exports.py"
cp "$stage/platform-player-unavailable-20261007.c" "$probe/user/platform-player-unavailable.c"
cp "$stage/elf-player-unavailable-20261007.c" "$probe/user/elf-player-unavailable.c"
cp "$stage/test_player_unavailable_20261007.py" "$probe/tests/test_player_unavailable.py"
for name in player decode_target window; do
 cp "$main/build/cobalt-player-audit-20261007/starboard-$name.h" "$probe/user/starboard-media/include/starboard/$name.h"
done
cd "$probe";mkdir -p build
printf 'PLAYER_UNAVAILABLE_TREE=%s\n' "$probe"
printf 'RUNNING\n' >player-unavailable.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >player-unavailable.status' EXIT
bash build.sh >player-unavailable-build.log 2>&1
python3 tests/test_player_unavailable.py >player-unavailable-bios.log 2>&1
python3 tools/build_uefi.py >player-unavailable-uefi-build.log 2>&1
python3 tests/test_player_unavailable.py --uefi >player-unavailable-uefi.log 2>&1
python3 "$stage/census_clean_imports_20261007.py" "$probe" >player-unavailable-census.log
tail -n 1 player-unavailable-bios.log
tail -n 1 player-unavailable-uefi.log
cat player-unavailable-census.log
