"""Native packed allocator collision/wrap/OOM/concurrency and exact resources."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'));sys.path.insert(0,str(root/'tests'))
from make_asset_disk import create
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--uefi',action='store_true')
args=parser.parse_args()
base=root/'build/allocator-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='packed-',dir=base))
intrinsics=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
obj=out/'fixture.o';string=out/'string.o';library=out/'fixture.so'
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
    '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables','-nostdinc','-isystem',intrinsics,'-I'+str(root/'user/sdk'),
    '-Wall','-Wextra','-Werror']
for source,target in [('elf-allocator-packed.c',obj),('platform-sdk-string.c',string)]:
    subprocess.run(['gcc',*flags,'-c',str(root/'user'/source),'-o',str(target)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),str(obj),str(string)],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
before=digest(boot);image=out/'frozen-boot.img';shutil.copyfile(boot,image)
disk=out/'assets.img';create(disk,['packed='+str(library)])
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),
                                                   Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
print('Packed allocator:',out,flush=True)
vm=VM('allocator-packed-uefi' if args.uefi else 'allocator-packed',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        output=long_command(vm,'elf live packed',timeout=240)
        assert 'Packed allocator collision/wrap/packing/zero/alignment/8workers/OOM PASS' in output,output
        assert 'ELF exited with status 91' in output,output
        assert_live_resources(output)
    print('PASS: native packed allocator 3 launches/8workers/collision/wrap/packing/'
          'calloc reuse/alignment/OOM/realloc ownership/exact resources',out,flush=True)
finally:vm.close()
assert digest(boot)==before==digest(image)
