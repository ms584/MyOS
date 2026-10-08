#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
native=/home/xytron404/cobalt-closed-imports-xnVZeH
out=$(mktemp -d /home/xytron404/pc-symbols-XXXXXX)
printf 'SYMBOL_BUILD_ROOT %s\n' "$out"
for name in build.sh boot cpu memory include kernel drivers fs net gui video audio user shell security tools vendor; do cp -a "$native/$name" "$out/$name"; done
mkdir "$out/build"
cp "$native"/build/*.h "$out/build/"
cp "$stage/process-vm-batched-prune-native-20261008.c" "$out/memory/process.c"
cp "$stage/runtime-gs-errno-20261008.c" "$out/user/platform-runtime.c"
cp "$stage/scheduler-gs-errno-20261008.c" "$out/cpu/scheduler.c"
cp "$stage/vm-ledger-20261008.h" "$out/include/vm-ledger.h"
cd "$out"
TEST_CASE=0 PROCESS_TESTS=0 TEXT_ONLY=0 PROCESS_RUNTIME_EXTERNAL=1 bash build.sh >build-symbols.log 2>&1
# Do not classify PCs using this ELF unless the complete boot image is EXACT.
cmp build/myos-current.img "$native/build/named-shm-lifetime-6va16v40/frozen-boot.img"
sha256sum build/myos-current.img build/kernel.elf build/platform-runtime.elf build/platform-runtime.mrt
printf 'PASS bit-identical frozen image; matching ELF symbols at %s\n' "$out"
