"""Run sealed formatting inside MyOS; not full stdio or Cobalt acceptance."""
from pathlib import Path
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
obj=out/'fixture.o'
subprocess.run(['gcc',*build['flags'],'-Wno-error=format-truncation','-c',
                str(root/'user/elf-format.c'),'-o',str(obj)],check=True)
library=out/'format-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),str(obj),
    *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
disk=out/'assets.img';create(disk,[f'format={library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('format-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live format')
        assert 'ELF exited with status 79' in result,result
        assert 'native sealed formatter/UTF8/callback/overflow PASS' in result,result
        assert_live_resources(result)
    print('PASS: native sealed formatting, integer/float/long-double/hex/position '
          'conversions, bounded buffers/counts, UTF8/errors, allocated output, '
          'callback stream locking and fwrite overflow, relaunch/resources. '
          'NOT file/std streams or graphics/Cobalt acceptance.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
