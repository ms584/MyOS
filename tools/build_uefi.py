"""Build the same MyOS kernel with a native x86-64 UEFI/GOP loader."""
import os
from pathlib import Path
import subprocess
from fat_image import make_esp
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/uefi'
def run(*args):
    print('$ '+' '.join(map(str,args)),flush=True)
    subprocess.run(args,cwd=ROOT,env=dict(os.environ,SOURCE_DATE_EPOCH='946684800'),check=True)
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    subprocess.run(['bash','build.sh'],cwd=ROOT,env=dict(os.environ,TEST_CASE='0',PROCESS_TESTS='0',TEXT_ONLY='0'),check=True)
    kernel=(ROOT/'build/kernel.bin').read_bytes();font=(ROOT/'assets/myos-font.bin').read_bytes()
    assert 0<len(kernel)<=256*1024 and len(font)==4096
    (OUT/'uefi_assets.h').write_text('\n'.join('static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};' for name,data in [('kernel_blob',kernel),('font_blob',font)]))
    run('gcc','-I/usr/include/efi','-I/usr/include/efi/x86_64','-Iinclude','-I'+str(OUT),'-DGNU_EFI_USE_MS_ABI',
        '-fpic','-fshort-wchar','-mno-red-zone','-mgeneral-regs-only','-fno-stack-protector','-fno-builtin',
        '-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror','-Os','-c','boot/uefi.c','-o',str(OUT/'loader.o'))
    run('nasm','-f','elf64','boot/uefi_entry.asm','-o',str(OUT/'entry.o'))
    run('ld','-nostdlib','-znocombreloc','-T','/usr/lib/elf_x86_64_efi.lds','-shared','-Bsymbolic',
        '/usr/lib/crt0-efi-x86_64.o',str(OUT/'loader.o'),str(OUT/'entry.o'),'-L/usr/lib','-lefi','-lgnuefi','-o',str(OUT/'loader.so'))
    sections=[arg for section in ['.text','.rodata','.data','.sdata','.dynamic','.dynsym','.rel','.rela','.reloc'] for arg in ['-j',section]]
    run('objcopy',*sections,'-O','pei-x86-64','--subsystem=10',str(OUT/'loader.so'),str(OUT/'BOOTX64.EFI'))
    (ROOT/'build/myos-uefi.img').write_bytes(make_esp((OUT/'BOOTX64.EFI').read_bytes()))
    print('UEFI image: build/myos-uefi.img (32 MiB FAT16 ESP)',flush=True)
if __name__=='__main__':main()


