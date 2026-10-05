#!/usr/bin/env bash
set -euo pipefail
main=/home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
probe=$(mktemp -d /tmp/myos-file-map-XXXXXX)
for dir in cpu kernel include memory net fs drivers user tools tests;do cp -a "$main/$dir" "$probe/$dir";done
for dir in boot shell audio security gui video assets vendor third_party docs;do ln -s "$main/$dir" "$probe/$dir";done
cp "$main/build.sh" "$probe/"
cp "$stage/runtime-file-map.c" "$probe/user/platform-runtime.c"
cp "$stage/process-file-map.c" "$probe/memory/process.c"
cp "$stage/process-file-map.h" "$probe/include/process.h"
cp "$stage/syscall-file-map.c" "$probe/kernel/syscall.c"
cp "$stage/syscall-file-map.h" "$probe/include/syscall.h"
cp "$stage/vm-area.h" "$stage/vm-file-call.h" "$probe/include/"
printf 'PRIVATE_FILE_MAP=%s\n' "$probe"
mkdir -p "$probe/build/engine-evaluation" "$probe/build/renderer-provider-evaluation"
cp -a "$main/build/engine-evaluation/contracts" "$probe/build/engine-evaluation/"
cp "$main/build/engine-evaluation/libcobalt.so" "$probe/build/engine-evaluation/"
cp -a "$main/build/renderer-provider-evaluation/provider-84sokh6g" "$probe/build/renderer-provider-evaluation/"
cd "$probe"
PROCESS_TESTS=0 bash build.sh >file-map-build.log 2>&1 || { tail -n 40 file-map-build.log;exit 1; }
tail -n 2 file-map-build.log
python3 tools/build_uefi.py >file-map-uefi-build.log 2>&1
