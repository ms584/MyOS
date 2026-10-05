"""PRIVATE kernel snapshot and native musl interface APIs, not Internet proof."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='interface-',dir=ROOT/'build'))
specs=[]
for name,nic,configured,carrier in [('none',0,0,0),('unconfigured',1,0,1),('up',1,1,1),('down',1,1,0)]:
 obj=out/(name+'.o');lib=out/(name+'.so')
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin',
  '-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
  '-Wall','-Wextra','-Werror','-Iinclude',f'-DEXPECT_NIC={nic}',f'-DEXPECT_CONFIGURED={configured}',
  f'-DEXPECT_CARRIER={carrier}','-c','user/elf-interface.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(lib),str(obj)],check=True)
 specs.append(name+'='+str(lib))
disk=out/'assets.img';create(disk,specs)
uefi='--uefi' in sys.argv
for nic in (False,True):
 options=dict(ram=64,nic=nic,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM('interface'+('-nic' if nic else '-none')+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  def check(name):
   for _ in range(2):
    result=long_command(vm,'elf live '+name)
    assert 'native NIC/DHCP/interface chain/ABI/errors/free PASS' in result,result
    assert 'ELF exited with status 89' in result,result
    assert_live_resources(result)
  check('unconfigured' if nic else 'none')
  if nic:
   result=long_command(vm,'net up');assert 'DHCP configured' in result,result
   check('up')
   network=vm.command('human-monitor-command',{'command-line':'info network'})
   names=re.findall(r'^([^ :\n]+):.*type=nic',network,re.M);assert len(names)==1,network
   vm.command('set_link',{'name':names[0],'up':False});check('down')
   vm.command('set_link',{'name':names[0],'up':True});check('up')
 finally:vm.close()
print('PASS: PRIVATE interfaces no NIC/DHCP/carrier/errors/relaunch/exact resources.',out,flush=True)
