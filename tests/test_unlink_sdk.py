"""Native RAMFS orphan lifetime, cancellation and exact resource recovery."""
from pathlib import Path
import argparse
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
from create_data_disk import create_image
parser=argparse.ArgumentParser()
parser.add_argument('--boot',type=Path)
parser.add_argument('--uefi',action='store_true')
args=parser.parse_args()
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
archive=root/'build/renderer-evaluation/mesa-25.0.7.tar.xz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6'
subprocess.run(['tar','-xJf',str(archive),'-C',str(out),
    'mesa-25.0.7/src/util/anon_file.c','mesa-25.0.7/src/util/anon_file.h',
    'mesa-25.0.7/src/util/detect_os.h'],check=True)
source=out/'mesa-25.0.7/src/util'
mesa=out/'anon-file.o'
subprocess.run(['gcc',*build['flags'],'-I'+str(source),'-c',str(source/'anon_file.c'),'-o',str(mesa)],check=True)
environment=out/'environment.o'
subprocess.run(['gcc',*build['flags'],'-c',str(root/'user/platform-environment.c'),'-o',str(environment)],check=True)
assets=[]
for name,defines in [('unlink',[]),('unlink-cancel',['-DUNLINK_CANCEL=1']),
                     ('unlink-snapshot',['-DUNLINK_SNAPSHOT=1'])]:
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run(['gcc',*build['flags'],'-I'+str(source),*defines,'-c',str(root/'user/elf-unlink-sdk.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(mesa),str(environment),
        *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(f'{name}={library}')
disk=out/'assets.img';create(disk,assets)
boot=args.boot or root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:
    options.update(ram=128,boot_bus='ide',firmware=(
        Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('unlink-sdk-uefi' if args.uefi else 'unlink-sdk',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live unlink')
        assert 'ELF exited with status 84' in result,result
        assert 'native unlink open/dup/name reuse/last close PASS' in result,result
        assert_live_resources(result)
        cancel_live(vm,'unlink-cancel','unlink blocked reader ready')
    result=long_command(vm,'elf live unlink')
    assert 'ELF exited with status 84' in result,result
    assert_live_resources(result)
    print('PASS: unlinked open handles survive, dup/shared versus independent offsets, '
          'same-name/parent reuse, 128 final-close reclamations, blocked-pipe cancellation '
          'with orphan data and 4 relaunches/exact resources. NOT full POSIX/Cobalt.',out,flush=True)
finally:vm.close()
for large in (() if args.uefi else (False,True)):
    data=out/('data-v2.img' if large else 'data-v1.img');create_image(data,large=large)
    for marker in ('commit','reboot'):
        vm=VM(f'unlink-snapshot-{large}-{marker}',ram=64,boot_image=image,disk=data,asset_disk=disk)
        try:
            vm.wait('myos> ',90)
            result=long_command(vm,'elf live unlink-snapshot')
            assert 'ELF exited with status 85' in result,result
            assert f'native orphan snapshot {marker} PASS' in result,result
        finally:vm.close()
print('PASS: real Mesa anonymous files/cloexec/zero-extension/error cleanup; '
      + ('UEFI lifetime/cancellation/resources (snapshot covered separately in BIOS).'
         if args.uefi else 'v1/v2 snapshot skips live 8 KiB orphan, named data persists without resurrection.'),out,flush=True)
assert before==digest(boot)==digest(image)
