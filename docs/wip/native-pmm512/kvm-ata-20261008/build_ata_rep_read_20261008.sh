#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
baseline=/home/xytron404/pc-symbols-RtA49E
trial=$(mktemp -d /home/xytron404/ata-rep-read-XXXXXX)
printf 'ATA_REP_TRIAL_ROOT %s\n' "$trial"
for name in build.sh boot cpu memory include kernel drivers fs net gui video audio user shell security tools vendor; do cp -a "$baseline/$name" "$trial/$name"; done
cp -a /home/xytron404/myos/tests "$trial/tests"
cp -a /home/xytron404/myos/assets "$trial/assets"
mkdir "$trial/build"
cp "$baseline"/build/*.h "$trial/build/"
cp "$stage/ata-rep-read-20261008.c" "$trial/drivers/ata.c"
cd "$trial"
TEST_CASE=0 PROCESS_TESTS=0 TEXT_ONLY=0 PROCESS_RUNTIME_EXTERNAL=1 bash build.sh >build-rep-read.log 2>&1
sha256sum build/myos-current.img build/platform-runtime.mrt
cmp build/platform-runtime.mrt "$baseline/build/platform-runtime.mrt"
printf 'PASS build; runtime identical; trial root %s\n' "$trial"
