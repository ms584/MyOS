#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
main=/home/xytron404/myos
probe=$(mktemp -d /home/xytron404/fs-native-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
printf 'FS_NATIVE_TREE=%s\n' "$probe"
cp "$stage/build-fs-lifetime-20261007.sh" "$probe/build.sh"
cp "$stage/ramfs-identity-shared-20261007.c" "$probe/fs/ramfs.c"
cp "$stage/fs-identity-shared-20261007.h" "$probe/include/fs.h"
cp "$stage/record-lock.c" "$probe/fs/record-lock.c"
cp "$stage/record-lock.h" "$probe/include/record-lock.h"
cp "$stage/fs-operation-call-20261007.h" "$probe/include/fs-operation-call.h"
cp "$stage/fs-operation-syscall-20261007.inc" "$probe/kernel/fs-operation-syscall.inc"
cp "$stage/syscall-fs-lifetime-20261007.c" "$probe/kernel/syscall.c"
cp "$stage/syscall-fs-lifetime-20261007.h" "$probe/include/syscall.h"
cp "$stage/runtime-fs-lifetime-20261007.c" "$probe/user/platform-runtime.c"
cp "$stage/elf-fs-identity.c" "$probe/user/elf-fs-identity.c"
cp "$stage/test_fs_identity.py" "$probe/tests/test_fs_identity.py"
cd "$probe"
mkdir -p build
printf 'RUNNING\n' >fs-native.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >fs-native.status' EXIT
bash build.sh >fs-native-build.log 2>&1
python3 tests/test_fs_identity.py >fs-native-bios.log 2>&1
python3 tools/build_uefi.py >fs-native-uefi-build.log 2>&1
python3 tests/test_fs_identity.py --uefi >fs-native-uefi.log 2>&1
tail -n 1 fs-native-bios.log
tail -n 1 fs-native-uefi.log
