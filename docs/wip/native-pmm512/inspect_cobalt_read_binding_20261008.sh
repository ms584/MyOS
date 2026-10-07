#!/usr/bin/env bash
set -euo pipefail
for image in /home/xytron404/cobalt-closed-imports-xnVZeH/build/engine-evaluation/libcobalt.so /home/xytron404/cobalt-closed-imports-xnVZeH/build/renderer-provider-evaluation/provider-84sokh6g/provider.so; do
 printf '%s\n' "$image"
 nm -D "$image" | grep -E ' (read|__read|openat)$' || true
done
sed -n '147,176p' /home/xytron404/myos/kernel/elf-link.c
