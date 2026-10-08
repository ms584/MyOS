#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
base=/home/xytron404/myos
trial=${1:-}
if test -z "$trial"; then
trial=$(mktemp -d /home/xytron404/native-integrated-XXXXXX)
printf 'INTEGRATED_NATIVE_ROOT %s\n' "$trial"
for name in AGENTS.md build.sh boot cpu memory include kernel drivers fs net gui video audio user shell security tools vendor third_party assets tests; do cp -a "$base/$name" "$trial/$name"; done
mkdir -p "$trial/build" "$trial/docs"
cp "$base"/build/*.h "$trial/build/"
cp "$base"/docs/*.md "$base"/docs/*.json "$trial/docs/"
fi
test -d "$trial/build"
printf 'BUILD_INTEGRATED_ROOT %s\n' "$trial"
cp "$stage/integrated-kernel-20261008.c" "$trial/kernel/kernel.c"
cp "$stage/integrated-elf-20261008.c" "$trial/kernel/elf.c"
cp "$stage/integrated-syscall-20261008.c" "$trial/kernel/syscall.c"
cp "$stage/integrated-elf-20261008.h" "$trial/include/elf.h"
cp "$stage/integrated-os-20261008.h" "$trial/include/os.h"
cp "$stage/elf-launch-20261008.h" "$trial/include/elf-launch.h"
cp "$stage/vm-ledger-20261008.h" "$trial/include/vm-ledger.h"
cp "$stage/process-vm-batched-prune-clean-20261008.c" "$trial/memory/process.c"
cp "$stage/scheduler-gs-clean-20261008.c" "$trial/cpu/scheduler.c"
cp "$stage/runtime-gs-clean-20261008.c" "$trial/user/platform-runtime.c"
cp "$stage/integrated-process-test-20261008.c" "$trial/user/process-test.c"
cp "$stage/integrated-shell-20261008.c" "$trial/shell/shell.c"
cp "$stage/ata-rep-read-20261008.c" "$trial/drivers/ata.c"
cp "$stage/test-desktop-frame-fence-20261008.py" "$trial/tests/test_desktop.py"
cd "$trial"
gcc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude "$stage/elf-launch-unit-20261008.c" -o build/elf-launch-unit
ASAN_OPTIONS=detect_leaks=1 build/elf-launch-unit
bash build.sh >integrated-build.log 2>&1
python3 tools/build_uefi.py >integrated-uefi-build.log 2>&1
sha256sum build/kernel.bin build/platform-runtime.bin build/platform-runtime.mrt build/myos-current.img build/myos-uefi.img >integrated-normal.sha256
printf 'PASS integrated clean builds; not native execution or full regression\n'
