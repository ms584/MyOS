"""Native owned-image exit lifecycle, not SIGABRT or Cobalt execution."""
from pathlib import Path
import argparse,runpy,shutil,subprocess,sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'));sys.path.insert(0,str(root/'tools'))
from test_c11_sdk import digest
from test_system import VM
from test_elf_dynamic import long_command,assert_live_resources,cancel_live
from make_asset_disk import create
parser=argparse.ArgumentParser();parser.add_argument('--uefi',action='store_true')
args=parser.parse_args()
build=runpy.run_path(str(root/'tools/build_format_sdk.py'));out=build['out']
extra=[]
for name in ('exit','cxx-finalize'):
    obj=out/(name+'.o');subprocess.run(['gcc',*build['flags'],'-c',
        str(root/'user'/('platform-'+name+'.c')),'-o',str(obj)],check=True);extra.append(obj)
assets=[]
for name,define in [('main',''),('worker','WORKER_EXIT'),('constructor','CTOR_EXIT'),
        ('recursive','RECURSIVE_EXIT'),('fini','FINI_EXIT'),('return-fini','RETURN_FINI'),('raw','RAW_EXIT'),
        ('worker-raw','WORKER_RAW'),('loop','LOOP_FINI')]:
    obj=out/(name+'.o');library=out/(name+'.so')
    subprocess.run(['gcc',*build['flags'],*(['-D'+define+'=1'] if define else []),
        '-c',str(root/'user/elf-exit-sdk.c'),'-o',str(obj)],check=True)
    subprocess.run(['ld','-shared','--hash-style=both','-Bsymbolic','-z','now','-z','relro',
        '-z','noexecstack','-e','_start','-fini','image_fini','-o',str(library),str(obj),
        *map(str,extra),*map(str,build['objects']),*map(str,build['math']['objects'].values())],check=True)
    assert '(NEEDED)' not in subprocess.check_output(['readelf','-dW',str(library)],text=True)
    assets.append(name+'='+str(library))
disk=out/'assets.img';create(disk,assets)
boot=root/'build'/('myos-uefi.img' if args.uefi else 'myos-current.img')
image=out/'frozen-boot.img';before=digest(boot);shutil.copyfile(boot,image)
options=dict(ram=64,boot_image=image,asset_disk=disk)
if args.uefi:options.update(ram=128,boot_bus='ide',firmware=(
    Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('exit-sdk-uefi' if args.uefi else 'exit-sdk',**options)
try:
    vm.wait('myos> ')
    for _ in range(3):
        for name in ('main','worker','constructor','recursive','fini','return-fini','raw','worker-raw'):
            result=long_command(vm,'elf live '+name)
            code=78 if name in ('recursive','fini','return-fini') else -77 if 'raw' in name else 77
            assert 'ELF exited with status '+str(code) in result,result
            assert result.count('native exit finalizers PASS')==(0 if 'raw' in name else 1),result
            assert_live_resources(result)
        cancel_live(vm,'loop','native exit looping finalizer ready')
    print('PASS: native main/worker/constructor exit, atexit/FINI order and once-only '
          'progress, recursive exit status, raw _Exit skipping callbacks, blocked '
          'workers, looping-finalizer cancellation, 24 launches/exact resources. '+str(out),flush=True)
finally:vm.close()
assert before==digest(boot)==digest(image)
