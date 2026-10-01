"""Test actual Mesa environment reader against process-local MyOS SDK."""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create

base=root/'build/environment-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='native-',dir=base))
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out)],check=True)
source=out/'mesa-25.0.7'
for patch in ('mesa_thread_util_target.patch','mesa_build_target.patch'):
    for check in (True,False):
        subprocess.run(['git','apply',*(['--check'] if check else []),
            '--directory='+str(source.relative_to(root)),str(root/'tools'/patch)],
            cwd=root,check=True)
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
    '-nostdinc','-isystem',include,'-I'+str(root/'user/sdk'),
    '-I'+str(source/'include'),'-I'+str(source/'src'),
    '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
    '-Wall','-Wextra','-Werror']
objects=[]
for name,path in (('environment',root/'user/platform-environment.c'),
                  ('string',root/'user/platform-sdk-string.c'),('fixture',root/'user/elf-environment.c')):
    obj=out/(name+'.o');subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],check=True)
    objects.append(str(obj))
library=out/'environment-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),*objects],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'environment={library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
os.environ['MYOS_ENV_HOST_ONLY']='host-value-must-not-cross-boundary'
vm=VM('environment-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live environment')
        assert 'ELF exited with status 78' in result,result
        assert 'native Mesa environment concurrency/bounds/identity PASS' in result,result
        assert_live_resources(result)
    print('PASS: native Mesa secure_getenv/identity and guest-only copied environment, '
          'eight concurrent workers, stable retired pointers, 64-key/64KiB bounds, '
          'failure preservation, empty relaunch/exact resources. NOT credentials '
          'or graphics/Cobalt acceptance.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
