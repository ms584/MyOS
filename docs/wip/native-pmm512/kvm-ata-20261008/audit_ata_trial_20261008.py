"""Verify that the native A/B guest source differs only in the ATA reader."""
from pathlib import Path
import hashlib
import json
baseline = Path('/home/xytron404/pc-symbols-RtA49E')
trial = Path('/home/xytron404/ata-rep-read-S8iLch')
roots = 'boot cpu memory include kernel drivers fs net gui video audio user shell security tools vendor'.split()
def inventory(root):
    result = {}
    for directory in roots:
        for path in (root / directory).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts:
                result[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    result['build.sh'] = hashlib.sha256((root / 'build.sh').read_bytes()).hexdigest()
    return result
old, new = inventory(baseline), inventory(trial)
changes = {name: [old.get(name), new.get(name)] for name in old.keys() | new.keys()
           if old.get(name) != new.get(name)}
report = dict(baseline=str(baseline), trial=str(trial), inputs=len(old), changes=changes)
(trial / 'build/ata-trial-source-audit.json').write_text(json.dumps(report, indent=2))
assert set(changes) == {'drivers/ata.c'}, report
print('PASS: native source inputs differ only in drivers/ata.c', report, flush=True)
