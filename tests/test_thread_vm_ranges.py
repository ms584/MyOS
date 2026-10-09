"""Real guest pthread handles in middle/high VM pools, with exact cleanup."""
import argparse,tempfile,subprocess,shutil
from pathlib import Path
from test_system import VM,ROOT
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--boot-image',type=Path,required=True);p.add_argument('--runtime',type=Path,required=True)
p.add_argument('--expect-old-rejection',action='store_true');a=p.parse_args()
out=Path(tempfile.mkdtemp(prefix='thread-vm-ranges-',dir=ROOT/'build/media-evaluation'))
obj=out/'ranges.o';so=out/'ranges.so'
subprocess.run(['gcc','-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-Wall','-Wextra','-Werror','-c','user/elf-thread-vm-ranges.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start','-o',str(so),str(obj)],check=True)
disk=out/'assets.img';create(disk,[f'ranges={so}'],runtime=a.runtime)
boot=out/'boot.img';shutil.copyfile(a.boot_image,boot)
vm=VM('thread-vm-ranges',ram=64,asset_disk=disk,boot_image=boot)
try:
 vm.wait('myos> ')
 for wave in range(2):
  output=long_command(vm,'elf live ranges')
  if a.expect_old_rejection:
   assert 'native expanded pthread getattr rejected' in output and 'ELF exited with status 1' in output,output
  else:
   assert 'native expanded pthread join/detach/getattr/guard PASS' in output and 'ELF exited with status 0' in output,output
  assert_live_resources(output)
 (out/'serial.log').write_text(vm.log.read_text())
 print('EXPECTED OLD REJECTION' if a.expect_old_rejection else 'PASS','real guest middle/high mmap pthread lifecycle and resources',out,flush=True)
finally:vm.close()
