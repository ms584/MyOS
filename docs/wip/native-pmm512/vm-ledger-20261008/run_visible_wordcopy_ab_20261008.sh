#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat /home/xytron404/pmm-syscall-R4sQZZ/clean-vm-ledger-retry.status)" = TERMINAL_EXIT=0
test "$(cat cobalt-wordcopy-memory-check.status)" = TERMINAL_EXIT=0
provider=$(cat wordcopy-provider-path.txt)
printf 'RUNNING\n' >visible-wordcopy-ab.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >visible-wordcopy-ab.status' EXIT
# Same frozen kernel/runtime/page/workload and1s screenshot sampling. No
# profiler and no concurrent system regression. Provider alone differs.
env MYOS_TLS_LAUNCHES=1 MYOS_RENDERER_PROVIDER_PATH="$provider" python3 "$stage/run_visual_latency_frozen_20261008.py" "$stage/test_cobalt_native_interactive_tls_20261008.py" "$PWD/build/named-shm-lifetime-6va16v40/frozen-boot.img" 978d15abe933379c1dd5a9a89fde59f08fe306d1a79a60bad41a06f70cca1313 >visible-wordcopy-trial.log 2>&1
env MYOS_TLS_LAUNCHES=1 MYOS_RENDERER_PROVIDER_PATH="$PWD/build/renderer-provider-evaluation/provider-84sokh6g" python3 "$stage/run_visual_latency_frozen_20261008.py" "$stage/test_cobalt_native_interactive_tls_20261008.py" "$PWD/build/named-shm-lifetime-6va16v40/frozen-boot.img" 978d15abe933379c1dd5a9a89fde59f08fe306d1a79a60bad41a06f70cca1313 >visible-wordcopy-baseline.log 2>&1
