#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
test "$(cat cobalt-gpu-frames.status)" = TERMINAL_EXIT=0
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing concurrent native VM'; exit 1
fi
printf 'RUNNING\n' >cobalt-wordcopy.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-wordcopy.status' EXIT
python3 "$stage/build_wordcopy_provider_20261008.py" >cobalt-wordcopy-provider-build.log 2>&1
provider=$(cat wordcopy-provider-path.txt)
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
python3 tools/build_uefi.py >cobalt-wordcopy-uefi-build.log 2>&1
python3 tests/test_renderer_provider.py --provider "$provider" >cobalt-wordcopy-stage-bios.log 2>&1
python3 tests/test_renderer_provider.py --provider "$provider" --uefi >cobalt-wordcopy-stage-uefi.log 2>&1
MYOS_RENDERER_PROVIDER_PATH="$provider" MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=2 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-wordcopy-interactive.log 2>&1
