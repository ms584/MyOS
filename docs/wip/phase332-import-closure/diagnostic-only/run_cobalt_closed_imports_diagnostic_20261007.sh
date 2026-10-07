#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
clean=/home/xytron404/player-unavailable-bI04LM
main=/home/xytron404/myos
test "$(cat "$clean/import-closure.status")" = TERMINAL_EXIT=0
probe=$(mktemp -d /home/xytron404/cobalt-closed-imports-XXXXXX)
git -C "$main" archive HEAD | tar -x -C "$probe"
for part in include fs kernel user tools memory cpu; do cp -a "$clean/$part/." "$probe/$part/"; done
cp "$clean/build.sh" "$probe/build.sh"
cp "$stage/signal-cobalt-fatal-trace-20261007.c" "$probe/kernel/signal.c"
cp "$stage/platform-window-diagnostic-20261007.c" "$probe/user/platform-window.c"
cp "$stage/test_startup_fonts_diagnostic_20261007.py" "$probe/tests/test_startup_fonts_diagnostic.py"
cd "$probe";mkdir -p build
ln -s "$main/build/engine-evaluation" build/engine-evaluation
ln -s "$main/build/renderer-provider-evaluation" build/renderer-provider-evaluation
printf 'COBALT_CLOSED_IMPORTS_TREE=%s\n' "$probe"
printf 'RUNNING\n' >cobalt-closed-imports.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-closed-imports.status' EXIT
bash build.sh >cobalt-closed-imports-build.log 2>&1
if nm build/platform-runtime.elf | grep -q MyOSMissing; then exit 1; fi
python3 "$stage/census_clean_imports_20261007.py" "$probe" >cobalt-closed-imports-census.log
python3 tests/test_startup_fonts_diagnostic.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-closed-imports.log 2>&1
tail -n 35 cobalt-closed-imports.log
