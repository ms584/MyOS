"""Native platform font assets/paths; NOT glyph rendering or Cobalt acceptance."""
from pathlib import Path
import subprocess,sys,tempfile,hashlib,json
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='platform-fonts-',dir=ROOT/'build'))
uefi='--uefi' in sys.argv
font_root=ROOT/'build/platform-fonts-20261007'
report=json.loads((font_root/'report.json').read_text())
for resource in report['resources']:
 data=(font_root/resource['name']).read_bytes()
 assert len(data)==resource['bytes'] and hashlib.sha256(data).hexdigest()==resource['sha256']
for absent in (0,1):
 obj=out/f'fonts{absent}.o';lib=out/f'fonts{absent}.so'
 subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
  '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Iuser/sdk','-Wall','-Wextra','-Werror',
  f'-DABSENT={absent}','-c','user/elf-platform-fonts.c','-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-z','noexecstack',
  '-e','_start','-o',str(lib),str(obj)],check=True)
 specs=['fonts='+str(lib)]
 if not absent:specs += ['systemfonts/'+item['name']+'='+str(font_root/item['name']) for item in report['resources']]
 disk=out/f'assets{absent}.img';create(disk,specs)
 options=dict(ram=64,boot_image=ROOT/'build'/('myos-uefi.img' if uefi else 'myos-current.img'),asset_disk=disk)
 if uefi:options.update(boot_bus='ide',firmware=(Path('/usr/share/OVMF/OVMF_CODE_4M.fd'),Path('/usr/share/OVMF/OVMF_VARS_4M.fd')))
 vm=VM(f'platform-fonts-{absent}'+('-uefi' if uefi else ''),**options)
 try:
  vm.wait('myos> ')
  for launch in range(3):
   result=long_command(vm,'elf live fonts')
   assert ('native font directories honestly unavailable PASS' if absent else 'native platform font directories/') in result,result
   assert 'ELF exited with status 90' in result,result
   assert_live_resources(result)
 finally:vm.close()
print('PASS native platform fonts paths/real sfnt assets/license/config/readonly/output guards/absent/relaunch/resources; NOT Cobalt glyph rendering.',out,flush=True)
