"""PRIVATE native abort-diagnostic fixture, never playback acceptance."""
import sys,subprocess,re,shutil,json,hashlib,tempfile
from pathlib import Path
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='abort-fixture-',dir=ROOT/'build'))
source=out/'fixture.c'
source.write_text(r"""
extern void SbSystemBreakIntoDebugger(void);
static void myos_abort_diagnostic(void *f,void *p){((void (*)(void *,void *))0x803f0014e3ull)(f,p);}
extern _Noreturn void abort(void);
static volatile unsigned returned;
__attribute__((noinline)) static void third(void){
#ifdef DIRECT
 abort();
#else
 SbSystemBreakIntoDebugger();
#endif
 ++returned;
}
__attribute__((noinline)) static void second(void){third();++returned;}
__attribute__((noinline)) static void first(void){second();++returned;}
void _start(void){
 myos_abort_diagnostic((void*)1,(void*)0);
 myos_abort_diagnostic((void*)0xfffffffffffffff8ull,(void*)0);
 first();++returned;
}
""")
symbols=subprocess.check_output(['nm','-n',str(ROOT/'build/platform-runtime.elf')],text=True)
match=re.search(r'^([0-9a-f]+) [Tt] myos_abort_diagnostic$',symbols,re.M)
assert match,'Diagnostic function symbol missing'
source.write_text(source.read_text().replace('0x803f0014e3ull','0x'+match.group(1)+'ull'))
specs=[]
for mode in ('break','direct'):
 obj=out/(mode+'.o');image=out/(mode+'.so')
 flags=['-DDIRECT'] if mode=='direct' else []
 subprocess.run(['gcc','-m64','-O1','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-omit-frame-pointer','-fno-optimize-sibling-calls',*flags,'-c',str(source),'-o',str(obj)],check=True)
 subprocess.run(['ld','-shared','-Ttext=0x108b000','--hash-style=both','-Bsymbolic','-z','now','-z','relro','-z','noexecstack','-e','_start',str(obj),'-o',str(image)],check=True)
 specs.append(mode+'='+str(image))
disk=out/'assets.img';create(disk,specs)
vm=VM('phase335-abort-diagnostic',ram=128,boot_image=ROOT/'build/myos-current.img',asset_disk=disk,cpu='max')
try:
 vm.wait('myos> ')
 for mode in ('break','direct','break','direct'):
  result=long_command(vm,'elf live '+mode)
  assert 'USER SIGNAL' in result and 'signal=6' in result,result
  assert 'USERFAULT' not in result and 'USER FAULT' not in result,result
  assert_live_resources(result)
  rows=re.findall(r'DIAG return-pcs([^\n]+)',result)
  assert len(rows)==(4 if mode=='break' else 3),(mode,rows)
  assert rows[:2]==[' 0x0000000000000000',' 0x0000000000000000'],rows
  pcs=[int(v,16) for v in rows[2].split()]
  assert len(pcs)>=4,rows
  assert sum(0x800108b000<=pc<0x800108d000 for pc in pcs)>=4,rows
  print('PASS real native callchain',mode,len(pcs),'bounded invalid frames/exact cleanup',flush=True)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
print('PASS diagnostic fixture only',out,flush=True)
