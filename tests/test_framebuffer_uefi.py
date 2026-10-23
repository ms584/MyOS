"""UEFI integration with a synthetic high framebuffer backed by QEMU DIMM RAM.
Only the reported GOP base is injected in generated test-only loader sources.
Production mapping, CR3 handoff, console/GUI drawing and Ring3 remain unchanged.
This is not a physical GPU result; normal GOP screenshots are tested separately.
"""
from pathlib import Path
import json,re,subprocess,sys,time
from test_system import VM,ROOT,OUT,symbols
from test_shell import command,type_text
from test_settings import leave,click
from test_calculator import sequence
sys.path.insert(0,str(ROOT/'tools'))
from fat_image import make_esp
HIGH=0x1001ff000
FLAGS=(1<<63)|0x9b
MASK=0x000ffffffffff000
FIRMWARE=('/usr/share/OVMF/OVMF_CODE_4M.fd','/usr/share/OVMF/OVMF_VARS_4M.fd')
def fixture(old=False):
    directory=OUT/('gop-high-old' if old else 'gop-high-new');directory.mkdir(exist_ok=True)
    source=(subprocess.check_output(['git','show','413c2e6:boot/uefi.c'],text=True)
            if old else (ROOT/'boot/uefi.c').read_text())
    line='UINT64 fb=gop->Mode->FrameBufferBase;'
    assert source.count(line)==1
    (directory/'loader.c').write_text(source.replace(line,f'UINT64 fb={HIGH:#x}ull; /* Synthetic test RAM, not real GOP base. */'))
    subprocess.run(['gcc','-I/usr/include/efi','-I/usr/include/efi/x86_64','-Iinclude','-Ibuild/uefi',
                    '-DGNU_EFI_USE_MS_ABI','-fpic','-fshort-wchar','-mno-red-zone','-mgeneral-regs-only',
                    '-fno-stack-protector','-fno-builtin','-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror','-Os',
                    '-c',str(directory/'loader.c'),'-o',str(directory/'loader.o')],check=True)
    subprocess.run(['ld','-nostdlib','-znocombreloc','-T','/usr/lib/elf_x86_64_efi.lds','-shared','-Bsymbolic',
                    '/usr/lib/crt0-efi-x86_64.o',str(directory/'loader.o'),'build/uefi/entry.o',
                    '-L/usr/lib','-lefi','-lgnuefi','-o',str(directory/'loader.so')],check=True)
    sections=[a for s in ['.text','.rodata','.data','.sdata','.dynamic','.dynsym','.rel','.rela','.reloc'] for a in ['-j',s]]
    subprocess.run(['objcopy',*sections,'-O','pei-x86-64','--subsystem=10',str(directory/'loader.so'),str(directory/'BOOTX64.EFI')],check=True)
    image=directory/'test.img';image.write_bytes(make_esp((directory/'BOOTX64.EFI').read_bytes()));return image

def value(vm,address):return int.from_bytes(vm.memory(address,8),'little')
def mapping(vm,root,va):
    table=root
    for shift in (39,30,21):
        entry=value(vm,table+((va>>shift)&511)*8)
        assert entry&1 and not entry&4,hex(entry)
        if shift==21:
            assert entry&128
            return entry
        table=entry&MASK

def main():
    old=fixture(True)
    vm=VM('gop-high-old-control',ram=128,boot_image=old,boot_bus='ide',firmware=FIRMWARE,high_framebuffer_memory=True)
    try:
        text=vm.wait('UEFI ERROR',30)
        assert 'UEFI EXIT BOOT SERVICES OK' not in text
        print('PASS: old production loader rejects the injected valid >4GiB framebuffer before handoff',flush=True)
    finally:vm.close()
    image=fixture()
    disk=OUT/('gop-high-app-data-'+str(time.time_ns())+'.img')
    subprocess.run([sys.executable,'tools/make_app_disk.py',str(disk),'build/loop.app','loop'],check=True)
    vm=VM('gop-high-mapped',ram=128,boot_image=image,boot_bus='ide',firmware=FIRMWARE,
          high_framebuffer_memory=True,disk=disk,disk_index=1)
    try:
        text=vm.wait('myos> ',35)
        assert 'UEFI HIGH FRAMEBUFFER MAPPED' in text and 'UEFI EXIT BOOT SERVICES OK' in text
        assert 'Console: framebuffer 800x600x32' in text
        addresses=symbols();boot=addresses['boot_state'];root=value(vm,boot+8)
        assert value(vm,boot+56)==HIGH
        virtual=0x7e00000000+(HIGH&0x1fffff)
        leaf=mapping(vm,root,virtual)
        assert (leaf&0x000fffffffe00000)==0x100000000
        assert (leaf&((1<<63)|0xf9f))==FLAGS,hex(leaf) # CPU may set accessed/dirty bits.
        assert value(vm,addresses['screen']+16)==virtual
        type_text(vm,'gfx\n');vm.wait('GRAPHICS DEMO READY')
        def pixel(x,y):return vm.memory(HIGH+y*3200+x*4,3)
        assert pixel(100,20)==bytes.fromhex('623b23')
        assert pixel(799,599)==bytes.fromhex('cfc423')
        vm.key('ret');vm.wait('Graphics demo closed')
        type_text(vm,'desktop\n');vm.wait('DESKTOP READY')
        assert pixel(200,300)==bytes.fromhex('2c1a11')
        vm.key('4');vm.wait('WINDOW OPEN 4')
        vm.key('5');vm.wait('WINDOW OPEN 5');sequence(vm,'7+8=','15')
        click(vm,addresses,742,27,'WINDOW OPEN 6');vm.wait('SNAKE SCORE 1',5)
        leave(vm)
        type_text(vm,'run /bin/loop\n')
        other=0;deadline=time.monotonic()+3
        while time.monotonic()<deadline:
            registers=vm.command('human-monitor-command',{'command-line':'info registers'})
            match=re.search(r'CR3=([0-9A-Fa-f]+)',registers)
            if match and int(match[1],16)!=root:other=int(match[1],16);break
            time.sleep(.01)
        assert other,'No active process CR3 observed'
        assert mapping(vm,other,virtual)==leaf
        vm.wait('error: app timed out')
        command(vm,'run /bin/hello','App exited with status 42')
        assert 'PANIC' not in vm.log.read_text()
        (OUT/'gop-high-result.json').write_text(json.dumps({'fixture':'synthetic high RAM, not physical GPU',
            'physical':hex(HIGH),'virtual':hex(virtual),'kernel_cr3':hex(root),'process_cr3':hex(other),
            'leaf':hex(leaf),'graphics_pixel_readback':True,'desktop_apps':True,'ring3_console':True},indent=2)+'\n')
        print('PASS: high physical framebuffer, non-large-page-aligned base, bounded UC/NX supervisor alias before memory_init; console/gfx pixel readback, three desktop apps and shared Ring3 CR3/console',flush=True)
    finally:vm.close()
if __name__=='__main__':main()
