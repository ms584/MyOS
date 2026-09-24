"""Build a 16 MiB raw BIOS HDD image containing the current MyOS desktop."""
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
SIZE=16*1024*1024
STAGE2_LBA=1
KERNEL_LBA=17

def run(*command):
    print('$ '+' '.join(map(str,command)),flush=True)
    subprocess.run(command,cwd=ROOT,check=True)

def main():
    # Always rebuild the normal image: never package a fault-test kernel.
    env=dict(os.environ,TEST_CASE='0',PROCESS_TESTS='0',TEXT_ONLY='0')
    print('$ bash build.sh',flush=True)
    subprocess.run(['bash','build.sh'],cwd=ROOT,env=env,check=True)
    kernel=(ROOT/'build/kernel.bin').read_bytes()
    if not kernel or len(kernel)>256*1024:
        raise SystemExit('Kernel must be nonempty and at most 256 KiB')
    sectors=(len(kernel)+511)//512
    boot=ROOT/'build/boot-hdd.bin'
    stage2=ROOT/'build/stage2-hdd.bin'
    run('nasm','-f','bin','-DBIOS_HDD=1','boot/boot.asm','-o',str(boot))
    run('nasm','-f','bin','-DBIOS_HDD=1',f'-DKERNEL_SECTORS={sectors}',
        'boot/stage2.asm','-o',str(stage2))
    first=boot.read_bytes();second=stage2.read_bytes()
    if len(first)!=512 or first[510:]!=b'\x55\xaa' or len(second)!=8192:
        raise SystemExit('Invalid BIOS loader size or boot signature')
    if KERNEL_LBA*512+len(kernel)>SIZE:
        raise SystemExit('Kernel does not fit the BIOS image')
    target=ROOT/'build/myos-bios.img'
    temp=ROOT/'build/myos-bios.img.tmp'
    try:
        with temp.open('wb') as image:
            image.truncate(SIZE)
            image.seek(0);image.write(first)
            image.seek(STAGE2_LBA*512);image.write(second)
            image.seek(KERNEL_LBA*512);image.write(kernel)
        os.replace(temp,target)
    finally:
        temp.unlink(missing_ok=True)
    print(f'BIOS HDD image: {target} ({SIZE} bytes; kernel {sectors} sectors)',flush=True)
    print('Boot with QEMU IDE primary master; use a separate primary-slave MyOS data disk for persistence.',flush=True)

if __name__=='__main__':main()
