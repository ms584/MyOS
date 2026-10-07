"""100 Hz sampled CPU clocks, not nanosecond precision or browser acceptance."""
from pathlib import Path
import subprocess,sys,tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='cpu-clock-',dir=ROOT/'build'))
obj=out/'clock.o';lib=out/'clock.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
 '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror',
 '-c','user/elf-cpu-clock.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
 '-e','_start','-o',str(lib),str(obj)],check=True)
disk=out/'assets.img';create(disk,['clock='+str(lib)])
uefi='--uefi' in sys.argv
options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
vm=VM('cpu-clock'+('-uefi' if uefi else ''),**options)
try:
 vm.wait('myos> ')
 for launch in range(3):
  result=long_command(vm,'elf live clock')
  assert 'native sampled CPU clocks' in result and 'ELF exited with status 89' in result,result
  assert_live_resources(result)
finally:vm.close()
print('PASS native CPU clock busy/sleep/process/thread/worker reuse/invalid clock guards/relaunch/exact resources; sampled 10ms, NOT browser.',out,flush=True)
