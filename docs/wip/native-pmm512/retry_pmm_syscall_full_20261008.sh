#!/usr/bin/env bash
set -euo pipefail
stage=/mnt/c/Users/Xytron/Documents/Codex/2026-09-30/myos-checkpoint-9f58024-home-xytron404-myos-2/work
cd /home/xytron404/pmm-syscall-R4sQZZ
test "$(cat pmm-syscall-full.status)" = TERMINAL_EXIT=1
cp -n pmm-syscall-full.log pmm-syscall-full-missing-core.log
cp -n pmm-syscall-full.status pmm-syscall-full-missing-core.status
cp /home/xytron404/shm-8m-4hJG2I/build/engine-evaluation/libcobalt.so build/engine-evaluation/libcobalt.so
python3 - <<'PY'
import hashlib,json
from pathlib import Path
audit=json.loads(Path('docs/PHASE33_COBALT_AUDIT.json').read_text())
assert hashlib.sha256(Path('build/engine-evaluation/libcobalt.so').read_bytes()).hexdigest()==audit['library_sha256']
assert Path('build/engine-evaluation/contracts/window.h').is_file()
print('Pinned Cobalt core hash and cached ABI window contract prerequisite verified')
PY
bash "$stage/run_pmm_syscall_full_20261008.sh"
