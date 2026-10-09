"""Guest transient allocation peak survives immediate free; read-only QMP."""
import argparse,tempfile,subprocess,shutil,struct,json
from pathlib import Path
from test_system import VM,ROOT,parse_symbols
from test_elf_dynamic import long_command,assert_live_resources
from make_asset_disk import create
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--image-dir',type=Path,required=True);a=p.parse_args()
out=Path(tempfile.mkdtemp(prefix='pmm-pressure-',dir=ROOT/'build/media-evaluation'))
obj=out/'pressure.o';so=out/'pressure.so'
subprocess.run(['gcc','-m64','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror','-c','user/elf-pmm-pressure.c','-o',str(obj)],check=True)
subprocess.run(['ld','-shared','--hash-style=both','-z','now','-z','relro','-e','_start','-o',str(so),str(obj)],check=True)
disk=out/'assets.img';create(disk,[f'pressure={so}'],runtime=a.image_dir/'platform-runtime.mrt')
boot=out/'boot.img';shutil.copyfile(a.image_dir/'myos-current.img',boot)
symbols=parse_symbols(subprocess.check_output(['nm',str(a.image_dir/'kernel.elf')],text=True))
vm=VM('pmm-pressure',asset_disk=disk,boot_image=boot,ram=64)
read=lambda n:struct.unpack('<Q',vm.memory(symbols[n],8))[0]
rows=[]
try:
 vm.wait('myos> ')
 for wave in range(2):
  before=read('free_count');start=read('ticks')
  output=long_command(vm,'elf live pressure')
  assert 'native transient 16MiB allocation/free PASS' in output and 'ELF exited with status 0' in output,output
  assert_live_resources(output)
  after=read('free_count')
  values=struct.unpack('<'+'QQ'*512,vm.memory(symbols['pmm_pressure_history'],8192))
  end=read('ticks')
  assert 0<=end-start<512,(start,end)
  minimum=min(values[i+1] for i in range(0,len(values),2) if start+1<=values[i]<=end+1)
  assert before==after,(before,after)
  assert minimum<=before-4096,(minimum,before)
  rows.append(dict(before_pages=before,after_pages=after,min_free_pages=minimum,start_tick=start,end_tick=end))
 (out/'serial.log').write_text(vm.log.read_text())
 (out/'evidence.json').write_text(json.dumps(rows,indent=2)+'\n')
 print('PASS read-only history captures transient >=16MiB with exact cleanup',out,rows,flush=True)
finally:vm.close()
