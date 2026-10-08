#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
test "$(cat cobalt-wordcopy.status)" = TERMINAL_EXIT=1
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing concurrent native VM'; exit 1
fi
provider=$(cat wordcopy-provider-path.txt)
printf 'RUNNING\n' >cobalt-wordcopy-memory-check.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-wordcopy-memory-check.status' EXIT
python3 tests/test_renderer_provider.py --provider build/renderer-provider-evaluation/provider-84sokh6g --uefi --ram 64 --expect-oom >cobalt-wordcopy-baseline-uefi64.log 2>&1
python3 tests/test_renderer_provider.py --provider "$provider" --uefi --ram 64 --expect-oom >cobalt-wordcopy-trial-uefi64.log 2>&1
python3 tests/test_renderer_provider.py --provider build/renderer-provider-evaluation/provider-84sokh6g --uefi --ram 256 >cobalt-wordcopy-baseline-uefi256.log 2>&1
python3 tests/test_renderer_provider.py --provider "$provider" --uefi --ram 256 >cobalt-wordcopy-trial-uefi256.log 2>&1
MYOS_RENDERER_PROVIDER_PATH="$provider" MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-wordcopy-interactive.log 2>&1
