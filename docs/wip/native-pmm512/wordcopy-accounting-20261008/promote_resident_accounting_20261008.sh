#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
candidate=/home/xytron404/pmm-syscall-R4sQZZ
test "$(cat "$candidate/clean-resident-accounting-full.status")" = TERMINAL_EXIT=0
python3 "$stage/audit_accounting_candidate_20261008.py" >resident-accounting-promotion-audit.json
for name in include/fs.h fs/ramfs.c kernel/elf.c; do cp "$candidate/$name" "$name"; done
cp "$candidate/tests/cobalt_native_resource_inventory.py" tests/cobalt_native_resource_inventory.py
cp "$stage/elf-named-shm-lifetime-20261008.c" user/elf-named-shm-lifetime.c
cp "$stage/test_named_shm_lifetime_production_20261008.py" tests/test_named_shm_lifetime.py
printf 'RUNNING\n' >resident-accounting-promotion.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >resident-accounting-promotion.status' EXIT
bash build.sh >resident-accounting-promotion-build.log 2>&1
python3 tools/build_uefi.py >resident-accounting-promotion-uefi-build.log 2>&1
sha256sum -c "$candidate/clean-resident-accounting-normal.sha256"
cp "$candidate/clean-resident-accounting-normal.sha256" build/resident-accounting-promotion-normal.sha256
python3 tests/test_named_shm_lifetime.py >resident-accounting-promotion-lifetime-bios.log 2>&1
python3 tests/test_named_shm_lifetime.py --uefi >resident-accounting-promotion-lifetime-uefi.log 2>&1
sha256sum -c build/resident-accounting-promotion-normal.sha256
git diff --check
