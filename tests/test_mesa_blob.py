"""Run real pinned Mesa blob utility with the MyOS SDK, not graphics acceptance."""
from pathlib import Path
import argparse
import hashlib
import runpy
import shutil
import subprocess
import sys
import tarfile
import tempfile

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--math-sdk',type=Path,help='Reuse verified math-only headers; no recompilation needed')
options=parser.parse_args()
if options.math_sdk:
    include=options.math_sdk.resolve()/'sdk/include'
    archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
    with archive.open('rb') as stream:
        assert hashlib.file_digest(stream,'sha256').hexdigest()=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
    with tarfile.open(archive) as source:
        for dest,original in (('math.h','include/math.h'),('features.h','include/features.h'),
                              ('fenv.h','include/fenv.h'),('bits/fenv.h','arch/x86_64/bits/fenv.h')):
            expected=source.extractfile('musl-1.2.6/'+original).read()
            if dest=='math.h':
                expected=expected.replace(b'#include <bits/alltypes.h>',b'#include <myos/math-types.h>')
            assert (include/dest).read_bytes()==expected,dest
    assert (include/'myos/math-types.h').read_bytes()==(root/'user/sdk/myos/math-types.h').read_bytes()
    assert {str(p.relative_to(include)) for p in include.rglob('*') if p.is_file()}=={
        'math.h','features.h','fenv.h','bits/fenv.h','myos/math-types.h'}
    base=root/'build/mesa-blob-evaluation';base.mkdir(parents=True,exist_ok=True)
    build={'out':Path(tempfile.mkdtemp(prefix='native-',dir=base)),'sdk_include':include}
else:
    build=runpy.run_path(str(root/'tools/build_math_sdk.py'))
out=build['out'];archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
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
       '-I'+str(build['sdk_include']),'-I'+str(source/'include'),'-I'+str(source/'src'),
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror','-Wno-unused-parameter']
objects=[]
for name,path in (('blob',source/'src/util/blob.c'),('string',root/'user/platform-sdk-string.c'),
                  ('fixture',root/'user/elf-mesa-blob.c')):
    obj=out/(name+'.o')
    subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],check=True)
    objects.append(str(obj))
library=out/'blob-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                '-z','noexecstack','-e','_start','-o',str(library),*objects],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'blob={library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('mesa-blob',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live blob')
        assert 'ELF exited with status 74' in result,result
        assert 'native Mesa blob growth/strings/bounds/OOM PASS' in result,result
        assert_live_resources(result)
    print('PASS: native real Mesa blob with SDK math headers, memchr/realloc growth, '
          'typed/string readback, fixed guards, truncated input, SIZE_MAX rejection, '
          'actual allocator exhaustion/trim failure ownership, relaunch/resources. '
          'NOT GLES/EGL/Cobalt acceptance.',flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
