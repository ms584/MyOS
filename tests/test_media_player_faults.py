"""Native player corrupt packet/OOM/lifecycle tests; not HTTPS fault closure."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,shutil,re,os
from test_system import ROOT,VM
from test_elf_dynamic import long_command
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
base=ROOT/'build/media-evaluation';provider=Path(sys.argv[1]).resolve()
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(provider/'provider.so')==json.loads((provider/'report.json').read_text())['provider_sha256']
out=Path(tempfile.mkdtemp(prefix='player-faults-',dir=base))
obj=out/'faults.o';lib=out/'faults.so'
subprocess.run([str(base/'target-cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-Iuser','-Iinclude','-I'+str(provider/'include'),'-Iuser/starboard-media/include','-Iuser/starboard-system/include',
 '-c','user/elf-media-player-faults.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
runtime=Path(os.environ.get('MYOS_MEDIA_RUNTIME_PATH',str(ROOT/'build/platform-runtime.mrt')))
disk=out/'assets.img';create(disk,['faults='+str(lib),'provider='+str(provider/'provider.so')],runtime=runtime)
boot=out/'boot.img';shutil.copyfile(Path(os.environ.get('MYOS_MEDIA_BOOT_IMAGE',str(base/'optin-dma-normal/myos-current.img'))),boot)
before=digest(boot)
sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
from qemu_kvm_optin_20261008 import install
install()
vm=VM('media-player-faults',ram=64,boot_image=boot,asset_disk=disk,audio=out/'audio.wav');outputs=[]
try:
 vm.wait('myos> ')
 for _ in range(2):
  output=long_command(vm,'elf pair-live faults provider',timeout=180);print(output,flush=True);outputs.append(output)
  assert 'native media focused lifecycle faults PASS' in output,output
  assert 'native media guest page exhaustion/recovery PASS' in output,output
  assert 'ELF exited with status 96' in output,output
  assert 'USER FAULT' not in output and 'PANIC' not in output,output
  m=re.search(r'ELF pair live: pages reclaimed; heap-before=(\d+) heap-after=(\d+)',output)
  assert m and m[1]==m[2],output
 output=long_command(vm,'run /bin/hello');assert 'App exited with status 42' in output,output
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
(out/'evidence.json').write_text(json.dumps(dict(passed=True,native_guest=True,ram_mib=64,
 provider_sha256=digest(provider/'provider.so'),core_sha256=digest(lib),boot_sha256=before,outputs=outputs),indent=2)+'\n')
print('PASS native corrupt VP8/Opus, repeated immediate destroy, rapid seek, OOM/create recovery and exact resources',out,flush=True)
