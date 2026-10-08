#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/cobalt-closed-imports-xnVZeH
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
grep -q '^TERMINAL_EXIT=' cobalt-close-repeat.status
if pgrep -af qemu-system | grep -F '/home/xytron404/cobalt-closed-imports-xnVZeH/build/'; then
 echo 'Refusing concurrent native VM'; exit 1
fi
cp "$stage/cobalt_gpu_frame_layout_20261008.c" tools/cobalt-gpu-frame-layout.c
cp "$stage/cobalt_gpu_frame_observer_20261008.py" tests/cobalt_gpu_frame_observer.py
cp "$stage/test_cobalt_native_interactive_tls_20261008.py" tests/test_cobalt_native_interactive_tls.py
gcc -std=c11 -Wall -Wextra -Werror -Wno-misleading-indentation -Iinclude tools/cobalt-gpu-frame-layout.c -o build/cobalt-gpu-frame-layout
python3 -m py_compile tests/cobalt_gpu_frame_observer.py tests/test_cobalt_native_interactive_tls.py
printf 'RUNNING\n' >cobalt-gpu-frames.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >cobalt-gpu-frames.status' EXIT
MYOS_TLS_GPU_FRAMES=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 MYOS_TLS_LAUNCHES=1 python3 tests/test_cobalt_native_interactive_tls.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 >cobalt-gpu-frames.log 2>&1
