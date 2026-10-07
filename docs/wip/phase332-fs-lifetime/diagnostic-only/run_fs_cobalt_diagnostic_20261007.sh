#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/fs-native-VzEALo
main=/home/xytron404/myos
test "$(cat "$clean/fs-runtime.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/fs-cobalt-diag-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$stage/build-fs-lifetime-trace-20261007.sh" "$probe/build.sh"
cp "$stage/platform-missing-fs-abort-20261007.asm" "$probe/user/platform-missing-abort.asm"
cp "$stage/platform-missing-abort.c" "$probe/user/platform-missing-abort.c"
cp "$stage/exports-cache-trace-20261007.py" "$probe/tools/make_runtime_exports.py"
cp "$stage/test_startup_diagnostic.py" "$probe/tests/test_startup_diagnostic.py"
cd "$probe"
mkdir -p build
ln -s "$main/build/engine-evaluation" build/engine-evaluation
ln -s "$main/build/renderer-provider-evaluation" build/renderer-provider-evaluation
ln -s /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n build/cobalt-resources-dwxalc2n
printf 'FS_COBALT_DIAG_TREE=%s\n' "$probe"
printf 'RUNNING\n' >fs-cobalt-diag.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >fs-cobalt-diag.status' EXIT
bash build.sh >fs-cobalt-diag-build.log 2>&1
python3 tests/test_startup_diagnostic.py build/cobalt-resources-dwxalc2n >fs-cobalt-diag.log 2>&1
tail -n 20 fs-cobalt-diag.log
