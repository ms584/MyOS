#!/usr/bin/env bash
set -euo pipefail
cd /home/xytron404/myos
trial=/home/xytron404/native-integrated-XZBHuo
test "$(cat "$trial/integrated-regression.status")" = TERMINAL_EXIT=0
test "$(cat "$trial/integrated-native.status")" = TERMINAL_EXIT=0
(cd "$trial" && sha256sum -c integrated-normal.sha256)
# Refuse an overlapping owner change or any drift from the audited inputs.
git diff --quiet -- . ':(exclude)docs/PROJECT_STATE.md' ':(exclude)docs/HANDOFF.md'
git diff --cached --quiet
python3 - <<'PY'
from pathlib import Path
import hashlib,json
root=Path.cwd(); trial=Path('/home/xytron404/native-integrated-XZBHuo')
audit=json.loads((root/'build/integrated-source-audit-20261008.json').read_text())
assert not audit['unexpected'] and not audit['missing_expected']
assert len(audit['changes'])==16
for name,(old,new) in audit['changes'].items():
    target=root/name; source=trial/name
    assert (hashlib.sha256(target.read_bytes()).hexdigest() if target.exists() else None)==old,name
    assert hashlib.sha256(source.read_bytes()).hexdigest()==new,name
print('PASS exact audited sixteen-file promotion preconditions')
PY
printf 'RUNNING\n' >build/native-integration-promotion.status
trap 'result=$?; printf "TERMINAL_EXIT=%s\n" "$result" >build/native-integration-promotion.status' EXIT
for file in drivers/ata.c memory/process.c include/vm-ledger.h include/elf-launch.h include/os.h include/elf.h kernel/elf.c kernel/syscall.c kernel/kernel.c cpu/scheduler.c user/process-test.c user/platform-runtime.c shell/shell.c tests/test_desktop.py tests/test_cobalt_native_acceptance.py tests/cobalt_pmm_observer.py; do
 cp "$trial/$file" "$file"
done
bash build.sh >build/native-integration-promotion-build.log 2>&1
python3 tools/build_uefi.py >build/native-integration-promotion-uefi-build.log 2>&1
cp "$trial/integrated-normal.sha256" build/native-integration-promotion-normal.sha256
sha256sum -c build/native-integration-promotion-normal.sha256
git diff --check
