"""High resident frames must remain usable by real safe-copy syscalls."""
from pathlib import Path
import hashlib,subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='pmm-syscall-',dir=ROOT/'build'))
obj=out/'fixture.o';lib=out/'fixture.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror','-c','user/elf-pmm-syscall.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack','-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['pmm_syscall='+str(lib)])
before=hashlib.sha256(disk.read_bytes()).hexdigest()
uefi='--uefi' in sys.argv;old='--expect-old-failure' in sys.argv
options=dict(ram=512,cpu='max',asset_disk=disk)
if uefi:options.update(boot_image=ROOT/'build/myos-uefi.img',boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('pmm-syscall'+('-uefi' if uefi else '-bios'),**options)
try:
    vm.wait('myos> ',timeout=120)
    for _ in range(1 if old else 2):
        result=long_command(vm,'elf live pmm_syscall',timeout=240)
        expected=3 if old else 93
        assert f'ELF exited with status {expected}' in result,result
        if not old:
            assert 'upper PMM syscall copy/read/write PASS' in result,result
            assert 'upper PMM permissions/preflight/deferred cleanup PASS' in result,result
        assert_live_resources(result)
finally:vm.close()
assert hashlib.sha256(disk.read_bytes()).hexdigest()==before
print('EXPECTED old high-frame rejection reproduced' if old else 'PASS actual upper-frame random/log, RO/NONE/supervisor/cross-page atomic failure, two relaunches/exact resources','UEFI' if uefi else 'BIOS',out,flush=True)
