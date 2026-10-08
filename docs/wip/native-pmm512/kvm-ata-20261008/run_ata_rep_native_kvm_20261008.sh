#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/ata-rep-read-S8iLch
native=/home/xytron404/cobalt-closed-imports-xnVZeH
mkdir -p build/engine-evaluation build/renderer-provider-evaluation/provider-84sokh6g docs
cp "$native/build/engine-evaluation/libcobalt.so" build/engine-evaluation/
cp "$native/build/renderer-provider-evaluation/provider-84sokh6g/provider.so" build/renderer-provider-evaluation/provider-84sokh6g/
cp "$native/build/renderer-provider-evaluation/provider-84sokh6g/report.json" build/renderer-provider-evaluation/provider-84sokh6g/
cp "$native/docs/PHASE33_COBALT_AUDIT.json" docs/
# Do not overlap the timed native observations. Validate the specific old QEMU
# process rather than trusting a stale lock/status file. No process is killed.
for ((attempt=0;attempt<36;attempt++)); do
    if ! test -r /proc/612261/cmdline; then break; fi
    if ! grep -aq 'native-verified-tls-k4zzj9e7/assets.img' /proc/612261/cmdline; then break; fi
    sleep 5
done
if test -r /proc/612261/cmdline && grep -aq 'native-verified-tls-k4zzj9e7/assets.img' /proc/612261/cmdline; then
    printf 'Original KVM VM still live; defer experiment without restart\n';exit 2
fi
cp build/myos-current.img build/ata-rep-frozen.img
env MYOS_TLS_EXPLICIT_KVM=1 MYOS_TLS_LAUNCHES=2 \
 MYOS_TLS_RUNTIME_ASSET_SOURCE=/home/xytron404/cobalt-closed-imports-xnVZeH/build/native-verified-tls-mrm5_amc/assets.img \
 MYOS_RENDERER_PROVIDER_PATH="$PWD/build/renderer-provider-evaluation/provider-84sokh6g" \
 python3 "$stage/run_visual_latency_frozen_20261008.py" "$stage/test_cobalt_native_interactive_tls_20261008.py" \
 "$PWD/build/ata-rep-frozen.img" 9858e435b9035cf0081b7cba6a7150956c73e8c6bfc2b1919ee4a55505d1ccc9 >native-ata-rep-kvm-retry.log 2>&1
