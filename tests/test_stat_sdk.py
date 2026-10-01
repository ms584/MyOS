"""Execute target stat/open wrappers and actual sealed Mesa file utility."""
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
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
    'mesa-25.0.7/src/util/os_file.c','mesa-25.0.7/src/util/os_file.h',
    'mesa-25.0.7/src/util/detect_os.h'],check=True)
source=out/'mesa-25.0.7'
for check in (True,False):
    subprocess.run(['git','apply',*(['--check'] if check else []),
        '--directory='+str(source.relative_to(root)),str(root/'tools/mesa_file_target.patch')],cwd=root,check=True)
objects=[]
for name,path in [('mesa',source/'src/util/os_file.c'),('fixture',root/'user/elf-stat-sdk.c')]:
    obj=out/(name+'.o')
    subprocess.run(['gcc',*build['flags'],'-I'+str(source/'src/util'),'-c',str(path),'-o',str(obj)],check=True)
    objects.append(obj)
library=out/'stat-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '--wrap=malloc','--wrap=realloc',
    '-z','noexecstack','-e','_start','-o',str(library),*map(str,objects),
    *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'stat={library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('stat-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live stat')
        assert 'ELF exited with status 83' in result,result
        assert 'native stat SDK/real Mesa file/metadata/dup/OOM PASS' in result,result
        assert_live_resources(result)
    print('PASS: native stat layout/guards/error rollback, file/dir/asset metadata, '
          'actual Mesa unique/open/read/dup/cloexec, binary contents, allocator OOM '
          'positive errno/descriptor rollback and relaunch/resources. NOT full stat/POSIX/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
