"""Ring 3 presentation, real PS/2 input, close/reuse and forced cleanup.

This proves the native window substrate, NOT EGL/GLES or Cobalt web execution.
"""
import argparse
import struct
import hashlib
import json
from pathlib import Path
import subprocess
import time
from test_system import VM, OUT, symbols
from test_shell import type_text
from test_elf_dynamic import assert_live_resources
from test_video import frame_shot
from make_asset_disk import create


def main(text=False,uefi=False):
    assert not (text and uefi)
    out=OUT/'native-ui';out.mkdir(exist_ok=True)
    manifest=json.loads(Path('docs/PHASE331_ABI_SOURCES.json').read_text())
    pin=next(item for item in manifest['headers'] if item['path'].endswith('/window.h'))
    assert hashlib.sha256(Path('build/engine-evaluation/contracts/window.h').read_bytes()).hexdigest()==pin['sha256']
    if not text:
        unit=out/'ui-unit'
        subprocess.run(['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                        '-Iinclude','kernel/native-ui.c','tests/native-ui-unit.c','-o',str(unit)],check=True)
        subprocess.run([str(unit)],check=True)
    # Use the pinned upstream window declarations, not the runtime's duplicate.
    headers=out/'include/starboard';headers.mkdir(parents=True,exist_ok=True)
    (headers/'export.h').write_text('#define SB_EXPORT\n')
    modes=(3,) if text else (0,1,2)
    images=[]
    for mode in modes:
        obj=out/f'ui-{mode}.o';image=out/f'ui-{mode}.so'
        subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                        '-Wall','-Wextra','-Werror','-Iinclude','-Ibuild/engine-evaluation/contracts',
                        f'-I{out}/include',f'-DMODE={mode}','-c','user/elf-native-ui.c','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start',
                        '-o',str(image),str(obj)],check=True)
        images.append(f'ui{mode}={image}')
    disk=out/('text-assets.img' if text else 'assets.img');disk.unlink(missing_ok=True);create(disk,images)
    options={}
    if uefi:
        options=dict(ram=128,boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
                     firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('native-ui-text' if text else ('native-ui-uefi' if uefi else 'native-ui'),asset_disk=disk,**options)
    try:
        vm.wait('myos> ',30)
        def wait(offset,marker,timeout=15):
            end=time.monotonic()+timeout
            while time.monotonic()<end:
                output=vm.log.read_text()[offset:]
                assert 'PANIC' not in output,output
                if marker in output:return output
                assert 'Native UI FAIL:' not in output,output
                time.sleep(.01)
            raise AssertionError((marker,vm.log.read_text()[offset:]))
        if text:
            offset=len(vm.log.read_text());type_text(vm,'elf live ui3\n')
            output=wait(offset,'myos> ')
            assert 'Native window text-mode unavailable PASS' in output,output
            assert 'ELF exited with status 55' in output,output;assert_live_resources(output)
            print('PASS: native window honestly unavailable in VGA text mode');return
        boot=vm.memory(symbols()['boot_state'],64)
        width,height,pitch,framebuffer=struct.unpack_from('<IIIQ',boot,44)
        assert (width,height)==(800,600)
        points=[(688,520),(700,532),(760,532),(700,568),(760,568),(783,583)]
        def actual():
            return [int.from_bytes(vm.memory(framebuffer+y*pitch+x*4,4),'little')&0xffffff for x,y in points]
        expected=[(688,520,(255,0,0)),(700,532,(255,0,0)),(760,532,(0,255,0)),
                  (700,568,(0,0,255)),(760,568,(255,255,0)),(783,583,(0x12,0x34,0x56))]
        for mode in (0,1,2,0):
            offset=len(vm.log.read_text())
            # Snapshot after the shell's launch diagnostics and immediately before
            # acquisition, using a kernel symbol breakpoint is unnecessary: the
            # chosen ROI is away from console text in this fixture.
            before=actual();type_text(vm,f'elf live ui{mode}\n')
            marker=('frame','cancel','fault')[mode]
            wait(offset,f'Native window {marker} ready\n')
            frame_shot(vm,f'{vm.name}-{mode}-present.ppm',expected)
            if mode==0:
                vm.key('shift','a');vm.key('down')
                vm.command('input-send-event',{'events':[
                    {'type':'rel','data':{'axis':'x','value':12}},
                    {'type':'rel','data':{'axis':'y','value':8}}]})
                vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':'left'}}]})
                time.sleep(.08)
                vm.command('input-send-event',{'events':[{'type':'btn','data':{'down':False,'button':'left'}}]})
                wait(offset,'Native window real input checked\n')
            elif mode==1:vm.key('esc')
            else:vm.key('b')
            output=wait(offset,'myos> ')
            if mode==0:
                assert 'Native window ABI/present/input/wait/close/reuse PASS' in output,output
                assert 'ELF exited with status 55' in output,output
            elif mode==1:assert 'ELF cancelled; all threads stopped' in output,output
            else:
                assert 'USER FAULT' in output and 'vector=6 ' in output,output
                assert 'error: invalid executable' in output,output
            assert_live_resources(output)
            assert actual()==before,('snapshot restoration',mode,before,actual())
        print('PASS: pinned SbWindow ABI, native RGBA pixels, atomic bad-row rejection, '
              'real key down/up/modifiers/arrows and mouse buttons, finite/blocking waits, '
              'close/reuse, fault/cancel/relaunch and exact page/heap/frame restoration')
    finally:vm.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi',action='store_true');args=parser.parse_args()
    main(uefi=args.uefi)
