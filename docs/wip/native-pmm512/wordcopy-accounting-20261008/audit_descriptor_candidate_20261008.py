from pathlib import Path
import subprocess,hashlib,json
main=Path('/home/xytron404/myos');candidate=Path('/home/xytron404/pmm-syscall-R4sQZZ')
rows=[]
for name in subprocess.check_output(['git','ls-files','-z'],cwd=main).decode().split('\0'):
 if not name or name.startswith('docs/'):continue
 a=main/name;b=candidate/name
 if a.is_file() and (not b.is_file() or a.read_bytes()!=b.read_bytes()):
  rows.append(dict(path=name,main=hashlib.sha256(a.read_bytes()).hexdigest(),candidate=hashlib.sha256(b.read_bytes()).hexdigest() if b.is_file() else None))
print(json.dumps(rows,indent=2))
