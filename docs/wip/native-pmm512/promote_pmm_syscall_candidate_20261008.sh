#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
candidate=/home/xytron404/pmm-syscall-R4sQZZ
test "$(cat "$candidate/pmm-syscall-full.status")" = TERMINAL_EXIT=0
test -z "$(git diff --name-only -- include kernel memory user tests build.sh)"
printf 'RUNNING\n' >build/pmm-syscall-promotion.status
trap 'printf "TERMINAL_EXIT=%s\n" "$?" >build/pmm-syscall-promotion.status' EXIT
# Exactly the reviewed differences. No private diagnostics/boot experiment.
for file in include/fs.h include/process.h include/shm-backing.h include/syscall.h kernel/syscall.c memory/memory.c memory/process.c tests/test_shared_memory.py user/elf-platform-socketpair.c user/elf-platform-system.c user/elf-server.c user/elf-shared-memory.c user/platform-runtime.c; do
 cp "$candidate/$file" "$file"
done
for file in tests/test_pmm_512m.py tests/test_pmm_syscall.py tests/test_vm_hint.py tests/test_fs_capacity.py user/elf-pmm-syscall.c user/elf-vm-hint.c user/elf-fs-capacity.c; do
 cp "$candidate/$file" "$file"
done
bash build.sh >build/pmm-syscall-promotion-build.log 2>&1
python3 tools/build_uefi.py >build/pmm-syscall-promotion-uefi-build.log 2>&1
python3 - <<'PY'
from pathlib import Path
import hashlib
candidate=Path('/home/xytron404/pmm-syscall-R4sQZZ')
for name in ('kernel.bin','platform-runtime.bin','myos-current.img','myos-uefi.img'):
 a=hashlib.sha256((Path('build')/name).read_bytes()).hexdigest()
 b=hashlib.sha256((candidate/'build'/name).read_bytes()).hexdigest()
 assert a==b,(name,a,b)
 print('EXACT full-tested normal candidate binary:',name,a)
PY
sha256sum build/kernel.bin build/platform-runtime.bin build/myos-current.img build/myos-uefi.img >build/pmm-syscall-promotion-normal.sha256
git diff --check
