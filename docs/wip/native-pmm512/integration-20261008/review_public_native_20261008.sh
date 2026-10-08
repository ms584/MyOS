#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
dest=docs/wip/native-pmm512/integration-20261008
review=$(mktemp -d build/native-public-review-XXXXXX)
tar -xzf "$dest/native-frames.tar.gz" -C "$review" --no-same-owner --no-same-permissions
cp "$dest/evidence.json" "$review/"
gzip -dc "$dest/native-serial.txt.gz" >"$review/serial.txt"
python3 "$dest/verify_native_web_bundle_20261008.py" "$review" >"$review/verification.log"
cp "$review/native-web-verification.json" "$dest/public-replay-verification.json"
printf 'PASS public native PNG/JSON/serial replay: %s\n' "$review"
