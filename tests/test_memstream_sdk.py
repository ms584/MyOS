"""Run owned memory streams and sealed real Mesa memstream wrapper in MyOS."""
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
    'mesa-25.0.7/src/util/memstream.c','mesa-25.0.7/src/util/memstream.h'],check=True)
source=out/'mesa-25.0.7/src/util'
objects=[]
for name,path in [('mesa',source/'memstream.c'),('fixture',root/'user/elf-memstream.c')]:
    obj=out/(name+'.o')
    subprocess.run(['gcc',*build['flags'],'-I'+str(source),'-c',str(path),'-o',str(obj)],check=True)
    objects.append(obj)
library=out/'memstream-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),*map(str,objects),
    *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'memstream={library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('memstream-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live memstream')
        assert 'ELF exited with status 81' in result,result
        assert 'native memory stream/real Mesa/seek/ownership/OOM/threads PASS' in result,result
        assert_live_resources(result)
    print('PASS: native dynamic memory streams and sealed actual Mesa wrapper; '
          'flush/close ownership, seek/overwrite/zero gaps, size publication, binary bytes, '
          'overflow/OOM rollback/recovery, 8-worker records and 3 relaunches/resources. '
          'NOT graphics/full stdio/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
