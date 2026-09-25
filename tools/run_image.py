"""Run a built MyOS BIOS, UEFI or USB image in QEMU."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from desktop_demo import data_disk
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--firmware',choices=['bios','uefi'],default='uefi')
    p.add_argument('--media',choices=['disk','usb'],default='disk')
    p.add_argument('--data',help='optional separate persistent MyOS data image')
    p.add_argument('--nic',choices=['rtl8139'],help='enable supported QEMU network card')
    args=p.parse_args()
    image=ROOT/'build'/('myos-usb.img' if args.media=='usb' else f'myos-{args.firmware}.img')
    if not image.is_file():raise SystemExit('Build images first: python3 tools/release.py build')
    cmd=['qemu-system-x86_64','-m','128M']
    if args.media=='usb':
        cmd+=['-drive',f'format=raw,file={image},if=none,id=bootusb','-usb','-device','usb-storage,drive=bootusb,bootindex=1']
    else:cmd+=['-drive',f'format=raw,file={image},if=ide,index=0','-boot','order=c']
    if args.firmware=='uefi':
        code=Path('/usr/share/OVMF/OVMF_CODE_4M.fd');template=code.with_name('OVMF_VARS_4M.fd')
        variables=ROOT/'build/run-uefi-vars.fd'
        if not variables.exists():shutil.copyfile(template,variables)
        cmd+=['-drive',f'if=pflash,format=raw,readonly=on,file={code}',
              '-drive',f'if=pflash,format=raw,file={variables}']
    if args.data:
        disk=(ROOT/args.data).absolute();data_disk(disk)
        index=0 if args.media=='usb' else 1
        cmd+=['-drive',f'format=raw,file={disk},if=ide,index={index}']
    if args.nic:cmd+=['-nic',f'user,model={args.nic}']
    cmd+=['-serial',f'file:{ROOT}/build/run-image-serial.log']
    subprocess.run([sys.executable,str(ROOT/'tools/run_logged.py'),'--',*cmd],cwd=ROOT,check=True)
if __name__=='__main__':main()
