"""Actual Ring 3 Starboard event pump, PS/2 input and visibility (not Cobalt)."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
from test_system import VM, OUT, symbols
from test_shell import type_text
from test_elf_dynamic import assert_live_resources
from test_video import frame_shot
from make_asset_disk import create


def main(uefi=False):
    out=OUT/'platform-events';out.mkdir(exist_ok=True)
    manifest=json.loads(Path('docs/PHASE331_ABI_SOURCES.json').read_text())
    headers=out/'include/starboard';headers.mkdir(parents=True,exist_ok=True)
    for name in ('event.h','input.h','key.h','window.h'):
        pin=next(item for item in manifest['headers'] if item['path'].endswith('/'+name))
        data=Path('build/engine-evaluation/contracts',name).read_bytes()
        assert hashlib.sha256(data).hexdigest()==pin['sha256'],name
        (headers/name).write_bytes(data)
    (headers/'export.h').write_text('#define SB_EXPORT\n#define SB_EXPORT_PLATFORM\n')
    (headers/'configuration.h').write_text('#define SB_C_FORCE_INLINE inline\n#define SB_API_VERSION 18\n')
    images=[]
    for mode in range(7):
        obj=out/f'events-{mode}.o';image=out/f'events-{mode}.so'
        subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                        '-Wall','-Wextra','-Werror','-Iinclude','-Ibuild/engine-evaluation/contracts',
                        f'-I{out}/include',f'-DMODE={mode}','-c','user/elf-platform-events.c','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        '-o',str(image),str(obj)],check=True)
        images.append(f'events{mode}={image}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,images)
    options={}
    if uefi:
        options=dict(ram=128,boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
                     firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('platform-events-uefi' if uefi else 'platform-events',asset_disk=disk,**options)
    try:
        vm.wait('myos> ',30)
        boot=vm.memory(symbols()['boot_state'],64)
        width,height,pitch,framebuffer=struct.unpack_from('<IIIQ',boot,44)
        assert (width,height)==(800,600)
        points=[(688,520),(700,532),(760,532),(700,568),(760,568),(783,583)]
        def actual():
            return [int.from_bytes(vm.memory(framebuffer+y*pitch+x*4,4),'little')&0xffffff for x,y in points]
        def wait(offset,marker,timeout=20):
            end=time.monotonic()+timeout
            while time.monotonic()<end:
                output=vm.log.read_text()[offset:]
                assert 'PANIC' not in output and 'Native events FAIL:' not in output,output
                if marker in output:return output
                time.sleep(.01)
            raise AssertionError((marker,vm.log.read_text()[offset:]))
        def shot(name,colors):
            assert actual()==colors,(name,actual(),colors)
            expected=[(x,y,((c>>16)&255,(c>>8)&255,c&255)) for (x,y),c in zip(points,colors)]
            frame_shot(vm,f'{vm.name}-{name}.ppm',expected)
        def key(code,down):
            return {'type':'key','data':{'down':down,'key':{'type':'qcode','data':code}}}
        for run,mode in enumerate((0,1,2,3,4,5,6,0)):
            offset=len(vm.log.read_text());before=actual();type_text(vm,f'elf live events{mode}\n')
            if mode in (1,2):
                wait(offset,'Native events '+('cancel' if mode==1 else 'fault')+' ready\n')
                shot(f'{run}-live',[0xff0000]*6)
                vm.key('esc' if mode==1 else 'b')
            elif mode==5:
                wait(offset,'Native events infinite ready\n')
            elif mode==6:
                wait(offset,'Native events explicit blurred ready\n');shot(f'{run}-blurred',[0xff0000]*6)
                vm.key('x');wait(offset,'Native events focused ready\n')
            else:
                if mode==0:
                    wait(offset,'Native events frame ready\n');shot(f'{run}-live',[0xff0000]*6)
                    for codes in (('shift','a'),('down',),('ctrl_r',),('alt_r',),('meta_r',)):
                        vm.key(*codes)
                    vm.command('input-send-event',{'events':[
                        {'type':'rel','data':{'axis':'x','value':12}},
                        {'type':'rel','data':{'axis':'y','value':8}}]})
                    for button in ('left','right','middle'):
                        for down in (True,False):
                            vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':button}}]})
                            time.sleep(.06)
                    wait(offset,'Native events real input checked\n')
                    wait(offset,'Native events hidden ready\n');shot(f'{run}-hidden',before)
                    vm.key('b')
                elif mode==3:
                    wait(offset,'Native events preload ready\n');shot(f'{run}-preload',before)
                    vm.key('b')
                wait(offset,'Native events reveal blurred ready\n')
                shot(f'{run}-revealed',[0x00ff00 if mode==0 else 0xff0000]*6)
                vm.key('x')
                wait(offset,'Native events focused ready\n')
                if mode==0:
                    vm.command('input-send-event',{'events':[key('c',True),
                        {'type':'btn','data':{'down':True,'button':'left'}}]})
            output=wait(offset,'myos> ')
            if mode in (0,3,4,5,6):
                assert 'Native events ABI/schedule/cancel/main-thread/lifecycle/input PASS' in output,output
                assert 'ELF exited with status 56' in output,output
            elif mode==1:assert 'ELF cancelled; all threads stopped' in output,output
            else:assert 'USER FAULT' in output and 'vector=6 ' in output,output
            assert_live_resources(output);assert actual()==before,('restore',mode,before,actual())
            if mode==0:
                vm.command('input-send-event',{'events':[key('c',False),
                    {'type':'btn','data':{'down':False,'button':'left'}}]})
        print('PASS: pinned SbEvent/SbInputData ABI, bounded/overflow-safe deadlines and IDs, '
              'main-thread callbacks, cross-thread schedule/cancel/wake, focused real PS/2 input, '
              'actual conceal/hidden presentation/reveal, preload, ordered graceful stop, '
              'fault/cancel/relaunch and exact page/heap/frame reclamation')
    finally:vm.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
    main(uefi=args.uefi)
