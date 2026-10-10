"""PRIVATE real native memory/thread failure diagnostic validation; no playback acceptance."""
import subprocess,tempfile,shutil,re,json,hashlib,sys
from pathlib import Path
from test_system import ROOT,VM
from test_elf_dynamic import long_command,assert_live_resources
sys.path.insert(0,str(ROOT/'tools'))
from make_asset_disk import create
out=Path(tempfile.mkdtemp(prefix='resource-fixture-',dir=ROOT/'build'))
source=out/'fixture.c'
source.write_text(r"""
#include <stdint.h>
#include <stddef.h>
#include "thread-limit.h"
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int);
extern void *malloc(size_t);
extern int *__errno_location(void);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *);
extern int pthread_join(uint64_t,void **);
extern void SbLogRaw(const char *);
struct timespec {int64_t tv_sec,tv_nsec;};
extern int clock_nanosleep(int,int,const struct timespec *,struct timespec *);
static uint64_t workers[MYOS_WORKER_LIMIT];
static volatile unsigned release;
static void *worker(void *unused){(void)unused;struct timespec t={0,20000000};
 while(!__atomic_load_n(&release,__ATOMIC_ACQUIRE))clock_nanosleep(1,0,&t,0);return 0;}
#define CHECK(x) do{if(!(x)){SbLogRaw("resource diagnostic FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void){
 CHECK(mmap(0,256u*1024u*1024u,3,0x22,-1,0)==(void *)(uintptr_t)-1);
 CHECK(*__errno_location()==12);
 CHECK(malloc(256u*1024u*1024u)==0&&*__errno_location()==12);
 CHECK(mprotect((void *)1,4096,3)==-1&&*__errno_location()==22);
 for(unsigned i=0;i<30;++i)CHECK(mmap(0,4096,3,0,-1,0)==(void *)(uintptr_t)-1&&*__errno_location()==22);
 for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)CHECK(!pthread_create(&workers[i],0,worker,0));
 uint64_t extra;CHECK(pthread_create(&extra,0,worker,0)==11);
 __atomic_store_n(&release,1,__ATOMIC_RELEASE);
 for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)CHECK(!pthread_join(workers[i],0));
 CHECK(!pthread_create(&extra,0,worker,0)&&!pthread_join(extra,0));
 SbLogRaw("native real memory/thread failures, bounded logs, worker reuse PASS\n");return 90;
}
""")
obj=out/'fixture.o';image=out/'fixture.so'
subprocess.run(['gcc','-m64','-O1','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone','-fno-stack-protector','-fno-omit-frame-pointer','-fno-optimize-sibling-calls','-Iinclude','-c',str(source),'-o',str(obj)],check=True)
subprocess.run(['ld','-shared','-Ttext=0x108b000','--hash-style=both','-Bsymbolic','-z','now','-z','relro','-z','noexecstack','-e','_start',str(obj),'-o',str(image)],check=True)
disk=out/'assets.img';create(disk,['fixture='+str(image)])
vm=VM('phase335-resource-diagnostic',ram=128,boot_image=ROOT/'build/myos-current.img',asset_disk=disk,cpu='max')
try:
 vm.wait('myos> ')
 for attempt in range(2):
  result=long_command(vm,'elf live fixture')
  assert 'native real memory/thread failures, bounded logs, worker reuse PASS' in result,result
  assert 'USER FAULT' not in result and 'USERFAULT' not in result and 'USER SIGNAL' not in result,result
  assert_live_resources(result)
  rows=re.findall(r'DIAG resource-failure op=([^ ]+) error=(\d+) bytes=(\d+)',result)
  assert len(rows)==25,rows
  assert rows[0]==('mmap','12','268435456') and rows[1][0:2]==('allocator-map','12'),rows
  assert rows[2]==('mprotect-args','22','4096'),rows
  assert rows[-1][0:2]==('thread-create','11'),rows
  assert len(re.findall(r'DIAG return-pcs',result))==25,result
  print('PASS actual native ENOMEM/40worker EAGAIN,24memory+1thread records,worker reuse/exact cleanup',attempt+1,flush=True)
finally:vm.close();shutil.copyfile(vm.log,out/'serial.log')
print('PASS resource diagnostic fixture only',out,flush=True)
