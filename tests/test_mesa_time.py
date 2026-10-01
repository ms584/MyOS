"""Native pinned Mesa time utilities; no graphics/Cobalt acceptance claim."""
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
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create

build=runpy.run_path(str(root/'tools/build_time_sdk.py'));out=build['out']
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out)],check=True)
source=out/'mesa-25.0.7'
for name in ('mesa_c11_target.patch','mesa_thread_util_target.patch','mesa_build_target.patch','mesa_time_target.patch'):
    patch=(root/'tools'/name).read_text().replace('a/threads_posix.c','a/src/c11/impl/threads_posix.c').replace('b/threads_posix.c','b/src/c11/impl/threads_posix.c')
    for check in (True,False):
        subprocess.run(['git','apply',*(['--check'] if check else []),'--directory='+str(source.relative_to(root)),'-'],input=patch,text=True,cwd=root,check=True)
shutil.copyfile(root/'user/platform-mesa-time.c',source/'src/util/myos_time.c')
flags=build['flags']+['-I'+str(source/'include'),'-I'+str(source/'src'),
    '-DHAVE_PTHREAD=1','-DHAVE_STRUCT_TIMESPEC=1','-DMYOS_SDK_EXTERNAL_C23_TIME=1',
    '-Wno-unused-parameter','-Wno-error=sign-compare',
    '-Wno-error=unused-function'] # Upstream TIME_T_MAX and native-timedlock helper.
objects=list(map(str,build['objects']))
for name,path in (
    ('os-time',source/'src/util/os_time.c'),('condition',source/'src/util/cnd_monotonic.c'),
    ('c23-time',source/'src/c11/impl/time.c'),('threads',source/'src/c11/impl/threads_posix.c'),
    ('sleep',root/'user/platform-c11-time.c'),('string',root/'user/platform-sdk-string.c'),
    ('fixture',root/'user/elf-mesa-time.c')):
    obj=out/(name+'.o');subprocess.run(['gcc',*flags,'-c',str(path),'-o',str(obj)],check=True)
    objects.append(str(obj))
library=out/'time-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),*objects],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
cancel_object=out/'cancel.o';cancel_library=out/'cancel.so'
subprocess.run(['gcc',*flags,'-DTEST_TIME_CANCEL=1','-c',str(root/'user/elf-mesa-time.c'),
                '-o',str(cancel_object)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(cancel_library),*objects[:-1],str(cancel_object)],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(cancel_library)],text=True)
disk=out/'assets.img';create(disk,[f'time={library}',f'cancel={cancel_library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('mesa-time',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live time')
        assert 'ELF exited with status 75' in result,result
        assert 'native Mesa clocks/calendar/32-bit waits PASS' in result,result
        assert_live_resources(result)
    cancel_live(vm,'cancel','native Mesa time cancel ready',timeout=15)
    result=long_command(vm,'elf live time')
    assert 'ELF exited with status 75' in result,result
    assert 'native Mesa clocks/calendar/32-bit waits PASS' in result,result
    assert_live_resources(result)
    print('PASS: native Mesa monotonic clocks/sleeps, finite/infinite unnotified '
          '32-bit waits, exact timespec carry, condition deadlines, UTC calendar '
          'negative epoch/leap/2038/overflow, eight blocked-worker cancellation, relaunch/resources. '
          'NOT timezone database, graphics or Cobalt acceptance.',flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
