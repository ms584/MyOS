"""Execute owned file/console stream semantics in MyOS, not web acceptance."""
from pathlib import Path
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
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
obj=out/'stream-fixture.o'
subprocess.run(['gcc',*build['flags'],'-c',str(root/'user/elf-stream.c'),'-o',str(obj)],check=True)
library=out/'stream-fixture.so'
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(library),str(obj),
    *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
cancel_obj=out/'stream-cancel.o';cancel_library=out/'stream-cancel.so'
subprocess.run(['gcc',*build['flags'],'-DSTREAM_CANCEL=1','-c',str(root/'user/elf-stream.c'),'-o',str(cancel_obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
    '-z','noexecstack','-e','_start','-o',str(cancel_library),str(cancel_obj),
    *map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(cancel_library)],text=True)
disk=out/'assets.img';create(disk,[f'stream={library}',f'stream-cancel={cancel_library}'])
boot=root/'build/myos-current.img';image=out/'frozen-boot.img';before=digest(boot)
shutil.copyfile(boot,image);assert before==digest(boot)==digest(image)
vm=VM('stream-sdk',ram=64,boot_image=image,asset_disk=disk)
try:
    vm.wait('myos> ')
    for _ in range(3):
        result=long_command(vm,'elf live stream')
        assert 'ELF exited with status 80' in result,result
        assert 'native stream file/console/ownership/thread/limit PASS' in result,result
        assert 'stream-console:'+'Z'*600+':end' in result,result
        assert 'stream-stderr:123' in result,result
        assert_live_resources(result)
        cancel_live(vm,'stream-cancel','stream blocked reader ready')
    # Check the last cancellation's file handle reclamation too.
    result=long_command(vm,'elf live stream')
    assert 'ELF exited with status 80' in result,result
    assert_live_resources(result)
    print('PASS: native unbuffered file/console streams, binary read/write, seek/EOF/errors, '
          'fd adoption/append/cloexec/close, 8-worker record serialization, descriptor limits, '
          '600-byte console chunking, allocation-OOM rollback, blocked pipe cancellation '
          'and 4 relaunches/resources. NOT full stdio/Cobalt.',out,flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
