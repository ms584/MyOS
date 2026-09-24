"""Build and launch the MyOS Desktop 1.0 QEMU demo with a safe data image."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

from create_data_disk import SIGNATURE,SIZE,create_image

ROOT=Path(__file__).resolve().parents[1]

def data_disk(path):
    if path.is_symlink():
        raise SystemExit(f'Refusing a symbolic link as demo data disk: {path}')
    if not path.exists():
        create_image(path)
        print(f'Created demo data disk: {path} ({SIZE} bytes)',flush=True)
        return
    if not path.is_file() or path.stat().st_size<SIZE or path.stat().st_size%512:
        raise SystemExit(f'Existing demo disk has an invalid size: {path}')
    with path.open('rb') as stream:
        if stream.read(len(SIGNATURE))!=SIGNATURE:
            raise SystemExit(f'Existing file is not a MyOS data disk: {path}')
    print(f'Reusing existing demo data disk: {path}',flush=True)

def logged(*command):
    return [sys.executable,str(ROOT/'tools/run_logged.py'),'--',*map(str,command)]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only',action='store_true',
                        help='build and validate the demo without opening QEMU')
    parser.add_argument('--data',default='build/myos-demo-data.img',
                        help='dedicated MyOS data image (default: build/myos-demo-data.img)')
    args=parser.parse_args()
    disk=(ROOT/args.data).absolute() if not Path(args.data).is_absolute() else Path(args.data)
    data_disk(disk)
    env=dict(os.environ,TEST_CASE='0',PROCESS_TESTS='0',TEXT_ONLY='0')
    subprocess.run(logged('bash','build.sh'),cwd=ROOT,env=env,check=True)
    boot=ROOT/'build/myos-current.img'
    print(f'Desktop 1.0 ready: boot={boot} data={disk}',flush=True)
    if args.prepare_only:return
    subprocess.run(logged('qemu-system-x86_64','-m','64M',
                          '-drive',f'format=raw,file={boot},if=floppy',
                          '-drive',f'format=raw,file={disk},if=ide,index=0',
                          '-boot','order=a',
                          '-serial',f'file:{ROOT}/build/desktop-demo-serial.log'),
                   cwd=ROOT,env=env,check=True)

if __name__=='__main__':main()
