from pathlib import Path
import subprocess,json
root=Path('/home/xytron404/myos')
candidate=Path('/home/xytron404/pmm-syscall-R4sQZZ')
changed=[]
for name in subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0'):
    if not name or name.startswith('docs/'):continue
    a=root/name;b=candidate/name
    if not b.is_file() or a.read_bytes()!=b.read_bytes():changed.append(name)
print(json.dumps(dict(changed=changed),indent=2))
assert sorted(changed)==sorted(['.gitattributes','.gitignore','include/fs.h','fs/ramfs.c','kernel/elf.c']),changed
