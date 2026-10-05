#!/usr/bin/env bash
set -euo pipefail
main=/home/xytron404/myos
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
for probe in /tmp/myos-file-map-u9NQ2N /tmp/myos-sparse-regression-oH6bTA;do
    test -f "$probe/third_party/compiler-rt/emutls.c"
    mkdir -p "$probe/build/engine-evaluation" "$probe/build/renderer-provider-evaluation"
    cp -a "$main/build/engine-evaluation/contracts" "$probe/build/engine-evaluation/"
    cp "$main/build/engine-evaluation/libcobalt.so" "$probe/build/engine-evaluation/"
    cp -a "$main/build/renderer-provider-evaluation/provider-84sokh6g" "$probe/build/renderer-provider-evaluation/"
    test -f "$probe/build/engine-evaluation/contracts/window.h"
done
export FILE_MAP_FULL_LOG=file-map-full-retry.log
bash "$stage/run_file_map_full.sh"
