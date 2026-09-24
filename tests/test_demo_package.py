"""Verify Desktop 1.0 demo preparation preserves user data images."""
from hashlib import sha256
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/system-tests'
OUT.mkdir(parents=True,exist_ok=True)
disk=OUT/'phase20-package-check.img'
invalid=OUT/'phase20-invalid-check.img'
link=OUT/'phase20-symlink-check.img'

def prepare(path,success=True):
    result=subprocess.run([sys.executable,str(ROOT/'tools/desktop_demo.py'),
                           '--prepare-only','--data',str(path)],
                          cwd=ROOT,text=True,stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)
    assert (result.returncode==0)==success,result.stdout
    return result.stdout

def main():
    disk.unlink(missing_ok=True)
    invalid.unlink(missing_ok=True)
    link.unlink(missing_ok=True)
    first=prepare(disk)
    assert 'Created demo data disk' in first,first
    assert disk.stat().st_size==2*1024*1024
    before=sha256(disk.read_bytes()).digest()
    second=prepare(disk)
    assert 'Reusing existing demo data disk' in second,second
    assert sha256(disk.read_bytes()).digest()==before,'existing data disk changed'
    invalid.write_bytes(b'not a MyOS disk')
    output=prepare(invalid,success=False)
    assert 'invalid size' in output,output
    assert invalid.read_bytes()==b'not a MyOS disk'
    link.symlink_to(disk)
    output=prepare(link,success=False)
    assert 'symbolic link' in output,output
    assert sha256(disk.read_bytes()).digest()==before
    link.unlink()
    boot=ROOT/'build/myos-current.img'
    assert boot.stat().st_size==1474560,boot
    print('PASS: repeatable demo build, existing data preserved, invalid and linked files rejected')

if __name__=='__main__':main()
