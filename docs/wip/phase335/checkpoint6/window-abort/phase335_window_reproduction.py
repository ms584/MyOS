"""PRIVATE real native memory/thread failure diagnostic validation; no playback acceptance."""
import subprocess,tempfile,shutil,re,json,hashlib,sys
from pathlib import Path
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='window-repro-',dir=ROOT/'build'))
source=out/'fixture.c'
source.write_text(r"""
#include <stdint.h>
#include <stdbool.h>
struct size {int width,height;float ratio;};
struct options {struct size size;bool windowed;const char *name;};
extern void SbWindowSetDefaultOptions(struct options *);
extern void *SbWindowCreate(const struct options *);
extern bool SbWindowDestroy(void *);
extern bool SbWindowGetSize(void *,struct size *);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("window reproduction FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void){
 struct options o;SbWindowSetDefaultOptions(&o);
 CHECK(o.size.width==800&&o.size.height==600);
 void *main=SbWindowCreate(&o);CHECK(main);
 o.size.width=200;o.size.height=30;
 CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO small native window rejected while primary live\n");
 SbWindowSetDefaultOptions(&o);CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO second full-size native window EBUSY\n");
 struct size dimensions;CHECK(SbWindowGetSize(main,&dimensions)&&dimensions.width==800&&dimensions.height==600);
 CHECK(SbWindowDestroy(main));
 o.size.width=200;o.size.height=30;CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO small native window also rejected without primary\n");
 SbWindowSetDefaultOptions(&o);main=SbWindowCreate(&o);CHECK(main&&SbWindowDestroy(main));
 SbLogRaw("native window rejection reproduced, primary lifetime intact\n");return 90;
}
""")
obj=out/'fixture.o';image=out/'fixture.so'
subprocess.run(['gcc','-m64','-O1','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-omit-frame-pointer','-fno-optimize-sibling-calls','-Iinclude','-c',str(source),'-o',str(obj)],check=True)
subprocess.run(['ld','-shared','-Ttext=0x108b000','--hash-style=both','-Bsymbolic','-z','now','-z','relro','-z','noexecstack','-e','_start',str(obj),'-o',str(image)],check=True)
disk=out/'assets.img';create(disk,['fixture='+str(image)])
vm=VM('phase335-window-reproduction',ram=128,boot_image=ROOT/'build/myos-current.img',asset_disk=disk,cpu='max')
try:
 vm.wait('myos> ')
 for attempt in range(2):
  result=long_command(vm,'elf live fixture')
  assert 'native window rejection reproduced, primary lifetime intact' in result,result
  assert 'USER FAULT' not in result and 'USER SIGNAL' not in result,result
  assert_live_resources(result)
  rows=re.findall(r'DIAG window-create[^\n]+',result)
  assert len(rows)==5,rows
  assert rows[1].endswith('0xffffffffffffffa1') and rows[2].endswith('0xfffffffffffffff0'),rows
  assert rows[3].endswith('0xffffffffffffffa1'),rows
  print('PASS reproduction only: small windows ENOTSUP, second fullscreen EBUSY, exact cleanup',attempt+1,flush=True)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
print('PASS window rejection reproduced; NOT repaired',out,flush=True)
