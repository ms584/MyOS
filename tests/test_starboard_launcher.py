"""Native zero-entry Starboard launcher/SABI/stack gate; NOT Cobalt acceptance."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
from test_system import VM, OUT
from test_shell import type_text
from test_elf_dynamic import long_command, assert_live_resources
from make_asset_disk import create


def fixtures():
    out=OUT/'starboard-launcher';out.mkdir(exist_ok=True)
    manifest=json.loads(Path('docs/PHASE331_ABI_SOURCES.json').read_text())
    headers=out/'include/starboard';headers.mkdir(parents=True,exist_ok=True)
    for name in ('event.h','input.h','key.h','window.h'):
        pin=next(item for item in manifest['headers'] if item['path'].endswith('/'+name))
        data=Path('build/engine-evaluation/contracts',name).read_bytes()
        assert hashlib.sha256(data).hexdigest()==pin['sha256'],name
        (headers/name).write_bytes(data)
    (headers/'export.h').write_text('#define SB_EXPORT\n#define SB_EXPORT_PLATFORM\n')
    (headers/'configuration.h').write_text('#define SB_C_FORCE_INLINE inline\n#define SB_API_VERSION 18\n')
    specs=[]
    for mode in range(9):
        obj=out/f'launch{mode}.o';image=out/f'launch{mode}.so'
        subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
                        '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                        '-Wall','-Wextra','-Werror','-Iinclude','-Ibuild/engine-evaluation/contracts',
                        f'-I{out}/include',f'-DMODE={mode}','-c','user/elf-starboard-launcher.c','-o',str(obj)],check=True)
        subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro',
                        '-init','init_first','-fini','fini_last','-o',str(image),str(obj)],check=True)
        assert struct.unpack_from('<Q',image.read_bytes(),24)[0]==0
        specs.append(f'launch{mode}={image}')
    original=(out/'launch0.so').read_bytes()
    # Independently compare the descriptor emitted by the fixture's compiler
    # against the descriptor embedded by the pinned actual Cobalt build.
    cobalt=Path('build/engine-evaluation/libcobalt.so').read_bytes()
    audit=json.loads(Path('docs/PHASE33_COBALT_AUDIT.json').read_text())
    assert hashlib.sha256(cobalt).hexdigest()==audit['library_sha256'],'wrong pinned Cobalt build'
    sabi=cobalt[0x2e2efa:].split(b'\0',1)[0]
    assert len(sabi)==556 and sabi+b'\0' in original
    assert json.loads(sabi)['sb_api_version']==18
    phoff=struct.unpack_from('<Q',original,32)[0];phnum=struct.unpack_from('<H',original,56)[0]
    ph=[struct.unpack_from('<IIQQQQQQ',original,phoff+i*56) for i in range(phnum)]
    def fileoff(va):
        for p in ph:
            if p[0]==1 and p[3]<=va<p[3]+p[5]:return p[2]+va-p[3]
        raise AssertionError(hex(va))
    dynamic=next(p for p in ph if p[0]==2);tags={}
    for pos in range(dynamic[2],dynamic[2]+dynamic[5],16):
        tag,value=struct.unpack_from('<qQ',original,pos)
        if not tag:break
        tags[tag]=value
    symoff=fileoff(tags[6]);count=struct.unpack_from('<II',original,fileoff(tags[4]))[1]
    stringoff=fileoff(tags[5]);export={}
    for i in range(count):
        pos=symoff+i*24;name=struct.unpack_from('<I',original,pos)[0]
        text=original[stringoff+name:].split(b'\0',1)[0].decode()
        if text in ('SbEventHandle','GetEvergreenSabiString'):export[text]=pos
    handler=export['SbEventHandle'];getter=export['GetEvergreenSabiString'];bad=[]
    variants=(('weak',handler+4,'B',0x22),('data',handler+4,'B',0x11),
              ('hidden',handler+5,'B',2),('zero-size',handler+16,'Q',0),
              ('reserved-visibility',handler+5,'B',4),
              ('abs',handler+6,'H',0xfff1),('undefined',handler+6,'H',0),
              ('outside',handler+8,'Q',128*1024*1024),
              ('long-function',handler+16,'Q',128*1024*1024),
              ('missing',handler,'I',struct.unpack_from('<I',original,handler)[0]+1),
              ('duplicate',getter,'I',struct.unpack_from('<I',original,handler)[0]),
              ('getter-data',getter+4,'B',0x11))
    for name,pos,fmt,value in variants:
        data=bytearray(original);struct.pack_into('<'+fmt,data,pos,value)
        image=out/f'{name}.so';image.write_bytes(data);specs.append(f'{name}={image}');bad.append(name)
    data=bytearray(original);struct.pack_into('<Q',data,40,0);struct.pack_into('<HHH',data,58,0,0,0)
    image=out/'sectionless.so';image.write_bytes(data);specs.append(f'sectionless={image}')
    disk=out/'assets.img';disk.unlink(missing_ok=True);create(disk,specs)
    return disk,bad


def main(uefi=False):
    disk,bad=fixtures();options={}
    if uefi:
        options=dict(ram=128,boot_image=Path('build/myos-uefi.img'),boot_bus='ide',
                     firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
    vm=VM('starboard-launcher-uefi' if uefi else 'starboard-launcher',asset_disk=disk,**options)
    try:
        vm.wait('myos> ',30)
        result=long_command(vm,'elf live launch0')
        assert 'unsupported runtime feature' in result and 'launcher getter' not in result,result
        for name in bad:
            result=long_command(vm,'elf starboard '+name)
            assert 'Starboard launcher getter' not in result and 'Starboard launcher init' not in result,result
            assert 'ELF staged image released; not executed' in result,result
            assert 'USER FAULT' not in result and 'ELF exited' not in result,result
        for name in ('launch0','sectionless','launch1','launch2','launch3','launch4','launch5','launch6','launch7','launch0'):
            url=' https://myos.test/a?b=1&c=2' if name in ('launch0','sectionless') else ''
            offset=len(vm.log.read_text());type_text(vm,'elf starboard '+name+url+'\n')
            end=time.monotonic()+25
            while time.monotonic()<end:
                result=vm.log.read_text()[offset:]
                assert 'PANIC' not in result and 'Starboard launcher FAIL' not in result,result
                if name in ('launch5','launch6') and 'Starboard launcher '+('cancel' if name=='launch5' else 'input')+' ready' in result:
                    vm.key('esc' if name=='launch5' else 'a')
                    break
                if result.endswith('myos> '):break
                time.sleep(.03)
            else:raise AssertionError((name,vm.log.read_text()[offset:]))
            if name in ('launch5','launch6'):
                end=time.monotonic()+5
                while time.monotonic()<end:
                    result=vm.log.read_text()[offset:]
                    if result.endswith('myos> '):break
                    time.sleep(.03)
                else:raise AssertionError((name,result))
            assert_live_resources(result)
            if name=='launch1':
                assert 'SABI: mismatch; constructors not executed' in result and 'ELF exited with status 95' in result,result
                assert 'launcher init' not in result and 'launcher destructor' not in result,result
            elif name in ('launch2','launch3','launch4','launch7'):
                assert 'USER FAULT' in result,result
                assert ('vector=6 ' if name=='launch2' else 'vector=14 ') in result,result
                if name in ('launch3','launch4','launch7'):
                    error={'launch3':'0006','launch4':'0015','launch7':'0004'}[name]
                    address='800FE05000' if name=='launch4' else '800FE04000'
                    assert 'error=0x000000000000'+error in result,result
                    assert 'address=0x000000'+address in result,result
                assert 'launcher destructor' not in result,result
                if name in ('launch2','launch7'):assert 'launcher init' not in result,result
            elif name=='launch5':
                assert 'ELF cancelled; all threads stopped' in result and 'launcher destructor' not in result,result
            else:
                assert 'ELF exited with status 57' in result and 'Starboard launcher finalizer PASS' in result,result
                assert result.index('SABI: pinned')<result.index('launcher init')<result.index('launcher constructor'),result
                assert '256KiB stack/args PASS' in result and 'launcher lifecycle PASS' in result,result
                if name=='launch6':assert 'launcher real input PASS' in result,result
        print('PASS: native zero-entry Starboard launcher; pinned SABI before constructors; '
              'strong RX export validation/sectionless image; owned argv/link; sized NX main stack/guard; '
              'real input; graceful reverse finalizers; getter/guard/NX faults, cancel/relaunch and page/heap reclamation. '
              'This fixture is NOT Cobalt web acceptance.')
    finally:vm.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uefi',action='store_true')
    main(uefi=parser.parse_args().uefi)
