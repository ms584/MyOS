"""Guest native underlay pixels, validation and resource recovery; no host decode."""
from pathlib import Path
import argparse,json,hashlib,subprocess,tempfile,time
from PIL import Image
from test_system import VM,ROOT
from test_shell import type_text
from test_elf_dynamic import assert_live_resources
from make_asset_disk import create
p=argparse.ArgumentParser();p.add_argument('--boot-image',type=Path,required=True);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--uefi',action='store_true');p.add_argument('--absolute-pointer',action='store_true');a=p.parse_args()
out=Path(tempfile.mkdtemp(prefix='native-underlay-',dir=ROOT/'build/media-evaluation'))
specs=[]
for mode in (0,1,2):
 obj=out/f'fixture-{mode}.o';so=obj.with_suffix('.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror','-Iinclude',f'-DMODE={mode}','-c','user/elf-native-underlay.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start','-o',str(so),str(obj)],check=True)
 specs.append(f'underlay{mode}={so}')
disk=out/'assets.img';create(disk,specs,runtime=a.runtime)
options=dict(ram=64)
if a.uefi:options=dict(ram=128,boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('native-underlay-contract-uefi' if a.uefi else 'native-underlay-contract',asset_disk=disk,boot_image=a.boot_image,**options)
shots=[]
def wait(offset,marker):
 end=time.monotonic()+20
 while time.monotonic()<end:
  s=vm.log.read_text()[offset:]
  assert 'Underlay FAIL:' not in s and 'PANIC' not in s,s
  if marker in s:return s
  time.sleep(.02)
 raise AssertionError((marker,vm.log.read_text()[offset:]))
def shot(name,expected):
 ppm=out/(name+'.ppm');vm.command('screendump',{'filename':str(ppm)})
 im=Image.open(ppm).convert('RGB');png=ppm.with_suffix('.png');im.save(png);ppm.unlink()
 for xy,rgb in expected.items():assert im.getpixel(xy)==rgb,(name,xy,im.getpixel(xy),rgb)
 shots.append(png.name)
blend={(680,520):(17,34,51),(682,520):(191,0,0),(684,520):(0,255,0),(684,524):(255,255,0)}
clear={(680,520):(17,34,51),(682,520):(64,0,0),(684,520):(0,0,0)}
moved={(682,520):(64,0,0),(684,520):(255,0,0),(688,520):(0,0,0)}
# Outside the base rectangle initial foreground is an opaque snapshot.
# The moved underlay can only be visible through the retained transparent pixels.
moved={(682,520):(64,0,0),(684,520):(255,0,0),(686,524):(0,0,255)}
try:
 vm.wait('myos> ')
 if a.absolute_pointer:
  offset=len(vm.log.read_text());type_text(vm,'mouse absolute\n');wait(offset,'Mouse: absolute virtual coordinates enabled')
  assert any(d['current'] and d['absolute'] for d in vm.command('query-mice'))
 for cycle,mode in enumerate((0,1,2,0)):
  offset=len(vm.log.read_text());type_text(vm,f'elf live underlay{mode}\n')
  wait(offset,'Underlay blend ready');shot(f'{cycle}-blend',blend)
  if cycle==0:
   # Real PS/2 movement: the guest pointer must overlay retained video, and
   # moving it away must restore all original alpha/video pixel assertions.
   # Keep each delta within the existing validated PS/2 fixture range.
   if a.absolute_pointer:
    vm.command('input-send-event',{'events':[{'type':'abs','data':{'axis':axis,'value':round(value*32767/limit)}} for axis,value,limit in [('x',682,799),('y',520,599)]]})
   else:
    for axis,value in (('x',120),('x',120),('x',42),('y',100),('y',100),('y',20)):
     vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':axis,'value':value}}]})
   time.sleep(.05)
   shot('cursor-over-video',{(682,520):(16,16,16),(683,522):(255,255,255)})
   if a.absolute_pointer:
    vm.command('input-send-event',{'events':[{'type':'abs','data':{'axis':axis,'value':round(value*32767/limit)}} for axis,value,limit in [('x',400,799),('y',300,599)]]})
   else:
    for axis,value in (('x',-120),('x',-120),('x',-42),('y',-100),('y',-100),('y',-20)):
     vm.command('input-send-event',{'events':[{'type':'rel','data':{'axis':axis,'value':value}}]})
   time.sleep(.05);shot('cursor-moved-video-restored',blend)
  vm.key('n')
  if mode==1:
   wait(offset,'Underlay cancel ready');vm.key('esc')
  elif mode==2:wait(offset,'Underlay fault ready')
  else:
   for name,pixels in (('clear',clear),('move',moved),('show',moved)):
    wait(offset,'Underlay '+name+' ready');shot(f'{cycle}-{name}',pixels);vm.key('n')
   wait(offset,'Underlay reacquire ready');vm.key('n')
  result=wait(offset,'myos> ')
  if mode==0:assert 'Underlay validation/blend/clear/move/hide/show/reuse PASS' in result and 'ELF exited with status 55' in result,result
  elif mode==1:assert 'ELF cancelled; all threads stopped' in result,result
  else:assert 'USER FAULT' in result and 'vector=6 ' in result,result
  assert_live_resources(result)
 print('PASS: real Ring3 underlay pixels, guest cursor overlay/restoration, premultiplied alpha/occlusion, bad requests atomic, seek clear, move, conceal/show, stop/fault/relaunch and exact resources',out,flush=True)
finally:
 (out/'serial.log').write_text(vm.log.read_text());vm.close()
 (out/'evidence.json').write_text(json.dumps(dict(absolute_pointer=a.absolute_pointer,boot_sha256=hashlib.sha256(a.boot_image.read_bytes()).hexdigest(),runtime_sha256=hashlib.sha256(a.runtime.read_bytes()).hexdigest(),shots=shots),indent=2)+'\n')
