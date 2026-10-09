"""Native WebM/VP8/Opus decoding checkpoint, no playback claim."""
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,shutil,re
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
base=ROOT/'build/media-evaluation'
out=Path(tempfile.mkdtemp(prefix='native-decode-',dir=base))
cc=base/'target-cc'
build_report=json.loads((base/'codec-build.json').read_text())
for name,item in build_report['archives'].items():
 assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==item['sha256'],name
objects=[]
for name in ('media-webm','elf-media-decode'):
    obj=out/(name+'.o');objects.append(obj)
    subprocess.run([str(cc),'-std=c11','-O2','-Wall','-Wextra','-Werror',
      '-Wno-misleading-indentation','-Iuser','-Iinclude',
      '-I'+str(base/'libvpx-1.15.2'),'-I'+str(base/'opus-1.5.2/include'),
      '-Iuser/starboard-system/include','-c','user/'+name+'.c','-o',str(obj)],check=True)
support=json.loads((ROOT/'build/renderer-provider-evaluation/provider-84sokh6g/report.json').read_text())
archives=[]
for item in support['inputs']:
 path=Path(item['path'])
 if path.name in ('libmyos-format.a','libmyos-math.a','libmyos-jump.a'):
  assert hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']
  archives.append(str(path))
# Pure musl abs from the already hash-sealed math source package.
support_source=ROOT/'build/math-evaluation/target-9zfl2mt4/musl-1.2.6/src/stdlib/abs.c'
assert support_source.read_text().strip()=='#include <stdlib.h>\n\nint abs(int a)\n{\n\treturn a>0 ? a : -a;\n}'
obj=out/'abs.o'
subprocess.run([str(cc),'-O2','-c',str(support_source),'-o',str(obj)],check=True)
objects.append(obj)
library=out/'decode.so'
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
 '-z','noexecstack','-e','_start','-o',str(library),*map(str,objects),
 '--start-group',str(base/'vpx-target/libvpx.a'),str(base/'opus-target/libopus.a'),*archives,'--end-group'],check=True)
dynamic=subprocess.check_output(['readelf','-dW',str(library)],text=True)
assert '(NEEDED)' not in dynamic and '(TEXTREL)' not in dynamic
undefined=subprocess.check_output(['nm','-u',str(library)],text=True)
(out/'undefined.txt').write_text(undefined)
allow=set(re.findall(r'\bX\((\w+),',(ROOT/'build/platform-runtime-exports.h').read_text()))
strong={line.split()[-1] for line in undefined.splitlines() if line.split()[0]=='U'}
assert strong<=allow,sorted(strong-allow)
fixture=base/'opus-trimming-video-test.webm'
lock=json.loads((ROOT/'docs/PHASE333_MEDIA_LOCK.json').read_text())
assert hashlib.sha256(fixture.read_bytes()).hexdigest()==lock['fixture']['sha256']
bad=out/'bad.webm';bad.write_bytes(b'\x00')
truncated=out/'truncated.webm';truncated.write_bytes(fixture.read_bytes()[:len(fixture.read_bytes())//2])
variants=[]
for name in ('bad','truncated'):
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run([str(cc),'-std=c11','-O2','-Iuser','-Iinclude',
  '-I'+str(base/'libvpx-1.15.2'),'-I'+str(base/'opus-1.5.2/include'),
  '-Iuser/starboard-system/include','-DEXPECT_ERROR=-1',
  '-DMEDIA_PATH="/assets/'+name+'"','-c','user/elf-media-decode.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
  '-z','noexecstack','-e','_start','-o',str(lib),str(objects[0]),str(obj),str(objects[-1]),
  '--start-group',str(base/'vpx-target/libvpx.a'),str(base/'opus-target/libopus.a'),*archives,'--end-group'],check=True)
 variants.append(name+'test='+str(lib))

disk=out/'assets.img'
create(disk,['decode='+str(library),'media='+str(fixture),'bad='+str(bad),'truncated='+str(truncated),*variants])
boot=ROOT/'build/myos-current.img'
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=digest(boot);frozen=out/'boot.img';shutil.copyfile(boot,frozen)
if '--kvm' in sys.argv:
 sys.path.insert(0,str(ROOT/'docs/wip/native-pmm512/kvm-ata-20261008'))
 from qemu_kvm_optin_20261008 import install
 install()
vm=VM('media-native-decode',ram=128,boot_image=frozen,asset_disk=disk)
results=[]
try:
 vm.wait('myos> ')
 for _ in range(2):
  output=long_command(vm,'elf live decode',timeout=120)
  print(output,flush=True)
  assert 'native standard WebM VP8/Opus asset demux/decode PASS; NOT playback' in output,output
  assert 'ELF exited with status 98' in output,output
  assert_live_resources(output);results.append(output)
 metrics=[re.search(r'frames=(\d+) audio-packets=(\d+) samples=(\d+) signal=(\d+) y-hash=(\d+)',x).groups() for x in results]
 assert metrics[0]==metrics[1]
 assert tuple(map(int,metrics[0]))==(383,107,616320,757336,15537460444265374182)
 for name in ('bad','truncated'):
  output=long_command(vm,'elf live '+name+'test',timeout=30)
  assert 'native standard WebM malformed/truncated rejection PASS; NOT playback' in output,output
  assert 'ELF exited with status 99' in output,output
  assert_live_resources(output);results.append(output)
finally:
 vm.close();shutil.copyfile(vm.log,out/'serial.log');assert digest(boot)==before
(out/'evidence.json').write_text(json.dumps({'native_decode':True,'playback':False,'accelerator':'kvm' if '--kvm' in sys.argv else 'tcg',
 'fixture_sha256':digest(fixture),'library_sha256':digest(library),'boot_sha256':before,
 'runs':results},indent=2)+'\n')
print('PASS native standard-media asset decode/relaunch/resources; NOT playback',out,flush=True)
