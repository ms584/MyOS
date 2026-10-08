"""Compare tracked build/test inputs against the full-tested immutable candidate."""
from pathlib import Path
import subprocess,hashlib,json
candidate=Path('/home/xytron404/pmm-syscall-R4sQZZ')
changes=[]
for name in subprocess.check_output(['git','ls-files'],text=True).splitlines():
    path=Path(name)
    if name.startswith(('docs/','.')) or name in ('AGENTS.md','README.md'):continue
    other=candidate/path
    if not other.is_file():
        changes.append(dict(path=name,candidate='MISSING'));continue
    if path.read_bytes()!=other.read_bytes():changes.append(dict(path=name))
print(json.dumps(changes,indent=2))
