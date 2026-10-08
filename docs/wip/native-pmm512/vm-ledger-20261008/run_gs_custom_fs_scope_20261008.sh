#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat native-vm-ledger-gs.status)" = RUNNING
# Pin copies before the native experiment's restore trap can change images.
printf 'RUNNING\n' >gs-custom-fs-scope.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >gs-custom-fs-scope.status' EXIT
python3 "$stage/test_gs_custom_fs_20261008.py" >gs-custom-fs-bios.log 2>&1
test "$(cat native-vm-ledger-gs.status)" = RUNNING
python3 "$stage/test_gs_custom_fs_20261008.py" --uefi >gs-custom-fs-uefi.log 2>&1
