"""Allowlist combined candidate source against the unchanged main checkout."""
from pathlib import Path
import hashlib,json
baseline=Path('/home/xytron404/myos')
trial=Path('/home/xytron404/native-integrated-XZBHuo')
roots='boot cpu memory include kernel drivers fs net gui video audio user shell security tools vendor third_party tests'.split()
expected=set('''drivers/ata.c memory/process.c include/vm-ledger.h include/elf-launch.h
include/os.h include/elf.h kernel/elf.c kernel/syscall.c kernel/kernel.c
cpu/scheduler.c user/process-test.c user/platform-runtime.c shell/shell.c
tests/test_desktop.py tests/test_cobalt_native_acceptance.py
tests/cobalt_pmm_observer.py'''.split())
def inventory(root):
    result={}
    for directory in roots:
        for path in (root/directory).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts:
                result[str(path.relative_to(root))]=hashlib.sha256(path.read_bytes()).hexdigest()
    result['build.sh']=hashlib.sha256((root/'build.sh').read_bytes()).hexdigest()
    return result
old,new=inventory(baseline),inventory(trial)
helper='tests/cobalt_native_resource_inventory.py'
assert helper in old and old[helper]==new.get(helper), 'resource helper changed unexpectedly'
changes={name:[old.get(name),new.get(name)] for name in old.keys()|new.keys()
         if old.get(name)!=new.get(name)}
report=dict(baseline=str(baseline),trial=str(trial),baseline_inputs=len(old),trial_inputs=len(new),
            changes=changes,unexpected=sorted(set(changes)-expected),missing_expected=sorted(expected-set(changes)))
(baseline/'build/integrated-source-audit-20261008.json').write_text(json.dumps(report,indent=2))
assert not report['unexpected'] and not report['missing_expected'],report
print('PASS integrated source allowlist',len(changes),'changes',len(old),'baseline inputs',flush=True)
