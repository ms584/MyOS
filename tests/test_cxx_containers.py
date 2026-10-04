"""Actual GCC vector/hash providers with target no-exception allocation, not full C++."""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
from build_cxx_containers_sdk import build
parser=argparse.ArgumentParser();parser.add_argument('--configuration',type=Path,required=True)
parser.add_argument('--renderer-probe',type=Path,required=True)
parser.add_argument('--faults-only',action='store_true',help='Reuse unchanged positive evidence; check all fatal paths and one recovery launch')
parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
cfg=args.configuration.resolve();probe=args.renderer_probe.resolve()
assert cfg.is_relative_to((root/'build/cxx-target-evaluation').resolve())
assert probe.is_relative_to((root/'build/renderer-evaluation').resolve())
provider=probe/'libmyos-runtime-probe.a';assert provider.is_file()
sdk=build(cfg,provider);out=sdk['out'];flags=sdk['flags']
assets=[]
for name,defines in [('containers',[]),('length-fault',['-DCONTAINER_LENGTH_FAULT=1']),
                     ('range-fault',['-DCONTAINER_RANGE_FAULT=1']),
                     ('assert-fault',['-DCONTAINER_ASSERT_FAULT=1','-D_GLIBCXX_ASSERTIONS=1'])]:
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run(['g++',*flags,'-Wno-unused-function',*defines,'-c',str(root/'user/elf-cxx-containers.cpp'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--gc-sections','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
                    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(sdk['library'])],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(name+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cxx-containers-uefi' if args.uefi else 'cxx-containers',**options)
try:
    vm.wait('myos> ')
    for _ in range(0 if args.faults_only else 3):
        result=long_command(vm,'elf live containers',timeout=60)
        assert 'ELF exited with status 100' in result and 'vector/hash/rehash/destructors/alignment/8workers PASS' in result,result
        assert 'USER FAULT' not in result,result
        assert_live_resources(result)
    for name in ('length-fault','range-fault','assert-fault'):
        result=long_command(vm,'elf live '+name,timeout=30)
        if name=='assert-fault':
            assert 'native GCC precondition failed' in result and 'USER SIGNAL' in result and 'signal=6' in result,result
        else:assert 'USER FAULT' in result and 'vector=6 ' in result,result
        assert_live_resources(result)
    result=long_command(vm,'elf live containers',timeout=60)
    assert 'ELF exited with status 100' in result and '8workers PASS' in result,result
    assert_live_resources(result)
    print('PASS: actual GCC vector/unordered_map/prime rehash/lifetime/alignment/eight workers,'
          ' target no-exception length fault/relaunch/exact resources; NOT full C++/Mesa/Cobalt.',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
