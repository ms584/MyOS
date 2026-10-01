"""Run real Mesa rounding and SDK descriptor declarations in MyOS."""
from pathlib import Path
import hashlib
import runpy
import shutil
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create

build=runpy.run_path(str(root/'tools/build_math_sdk.py'));out=build['out']
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out)],check=True)
source=out/'mesa-25.0.7'
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
        '--directory='+str(source.relative_to(root)),str(root/'tools/mesa_thread_util_target.patch')],
        cwd=root,check=True)
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
    '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
    '-nostdinc','-isystem',include,'-I'+str(root/'user/sdk'),
    '-I'+str(build['sdk_include']),'-I'+str(source/'include'),'-I'+str(source/'src'),
    '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
    '-Wall','-Wextra','-Werror']
obj=out/'integer-fixture.o'
subprocess.run(['gcc',*flags,'-c',str(root/'user/elf-integer-sdk.c'),'-o',str(obj)],check=True)
library=out/'integer-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),str(obj),
    *map(str,build['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'integer={library}'])
boot=root/'build/myos-current.img';image=out/'integer-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('integer-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live integer')
        assert 'ELF exited with status 77' in result,result
        assert 'native integer headers/Mesa rounding/descriptors PASS' in result,result
        assert_live_resources(result)
    print('PASS: native Mesa LP64/SSE ties-even rounding and typed SDK file/pipe '
          'flags, shared dup offset, positioned read, EOF/errors, relaunch/resources. '
          'NOT runtime printf/scanf or graphics/Cobalt acceptance.',flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
