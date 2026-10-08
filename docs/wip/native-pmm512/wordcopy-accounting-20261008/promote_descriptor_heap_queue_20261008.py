from pathlib import Path
import subprocess,hashlib,json
root=Path('/home/xytron404/myos');candidate=Path('/home/xytron404/pmm-syscall-R4sQZZ')
expected={'drivers/sb16.c','include/fs.h','include/process.h','memory/process.c','tests/test_fs_capacity.py','user/elf-fs-capacity.c','user/elf-fs-runtime-lifetime.c','user/elf-platform-system.c','user/platform-runtime.c'}
assert (candidate/'descriptor-heap-queue-full.status').read_text().strip()=='TERMINAL_EXIT=0'
assert 'PASS: selected suite; normal framebuffer image restored' in (candidate/'descriptor-heap-queue-full.log').read_text()
different=set()
for name in subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0'):
 if not name or name.startswith('docs/') or name in ('.gitignore','.gitattributes'):continue
 a=root/name;b=candidate/name
 if a.is_file() and (not b.is_file() or a.read_bytes()!=b.read_bytes()):different.add(name)
assert different==expected,different
subprocess.run(['sha256sum','-c','descriptor-heap-queue-normal.sha256'],cwd=candidate,check=True)
for name in sorted(expected):subprocess.run(['cp',str(candidate/name),str(root/name)],check=True)
print('Exact tested9-file slice copied; private ELF/renderer/launcher/accounting NOT included',flush=True)
