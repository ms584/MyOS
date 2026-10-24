"""Production bulk wait: masked PIT, asynchronous DMA, MFINDEX and old-budget control."""
from pathlib import Path
import subprocess,tempfile,resource,signal
ROOT=Path(__file__).resolve().parents[1]
PREFIX=r"""
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
struct host;
static uint32_t test_read(uintptr_t);
static void test_write(uintptr_t,uint32_t);
static void test_poll_progress(struct host *);
volatile uint64_t ticks;
"""
SUFFIX=r"""
static struct host fixture;
static struct trb events[ENTRIES];
static unsigned polls,writes,after,per_frame,origin,event_code,event_residual;
static const uintptr_t expected=0xabc00;
static void test_poll_progress(struct host *h){
    assert(h==&fixture);++polls;
    if(after&&polls==after)events[0]=(struct trb){expected,0,
        (event_code<<24)|event_residual,TYPE(32)|(1u<<24)|(3u<<16)|1};
}
static uint32_t test_read(uintptr_t p){
    assert(p==0x2000);
    return (origin+(per_frame?polls/per_frame:0))&0x3fff;
}
static void test_write(uintptr_t p,uint32_t v){
    (void)v;assert(p==0x2038||p==0x203c);++writes; /* ERDP only. */
}
void print(const char *s){(void)s;}
void mouse_usb_report(uint8_t b,int x,int y){(void)b;(void)x;(void)y;assert(0);}
static bool run(unsigned delay,unsigned pace,unsigned start,unsigned code,unsigned residual,
                unsigned *reason,unsigned *elapsed){
    memset(&fixture,0,sizeof fixture);memset(events,0,sizeof events);
    fixture.interrupter=0x2020;fixture.events=(uintptr_t)events;fixture.event_cycle=1;
    polls=writes=0;after=delay;per_frame=pace;origin=start;event_code=code;event_residual=residual;
    ticks=123;bool result=wait_bulk(&fixture,expected,reason,elapsed);
    assert(ticks==123);return result; /* IRQ-driven clock never progresses. */
}
int main(void){
    unsigned reason,elapsed;
    assert(run(2400000,1000,0,1,0,&reason,&elapsed));
    assert(polls==2400000&&elapsed<8000&&writes==2&&!reason);
    assert(run(300000,1000,0x3f00,1,0,&reason,&elapsed)&&elapsed>=299&&elapsed<=300);
    assert(!run(0,1,0,1,0,&reason,&elapsed)&&reason==2&&elapsed==8000&&polls==8000&&!writes);
    assert(!run(0,0,0,1,0,&reason,&elapsed)&&reason==3&&!elapsed&&polls==2000000&&!writes);
    assert(!run(4,1,0,6,0,&reason,&elapsed)&&reason==1&&fixture.transfer_code==6&&writes==2);
    assert(!run(4,1,0,13,1,&reason,&elapsed)&&reason==1&&fixture.transfer_residual==1);
    assert(!run(4,1,0,1,1,&reason,&elapsed)&&reason==1); /* Still exact length. */
    assert(run(1,1,0,1,0,&reason,&elapsed)&&!reason&&!elapsed);
    return 0;
}
"""
def main():
    source=(ROOT/'drivers/xhci.c').read_text()
    read='static uint32_t read32(uintptr_t p){return *(volatile uint32_t *)p;}'
    write='static void write32(uintptr_t p,uint32_t v){*(volatile uint32_t *)p=v;}'
    assert source.count(read)==source.count(write)==1
    source=source.replace(read,'static uint32_t read32(uintptr_t p){return test_read(p);}')
    source=source.replace(write,'static void write32(uintptr_t p,uint32_t v){test_write(p,v);}')
    assert source.count('static void poll(struct host *h){')==1
    source=source.replace('static void poll(struct host *h){','static void poll(struct host *h){test_poll_progress(h);')
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='usb-bulk-clock-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(PREFIX+source+SUFFIX)
        flags=['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include')]
        subprocess.run([*flags,'-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        first=source.index('static bool wait_bulk(');last=source.index('\nbool usb_bulk_transfer(',first)
        old=source[:first]+"""static bool wait_bulk(struct host *h,uintptr_t wanted,unsigned *reason,unsigned *elapsed){
            *reason=0;*elapsed=0;uint64_t end=ticks+100;
            for(unsigned n=0;n<2000000&&ticks<end;++n){
                poll(h);if(h->done_transfer==wanted)return h->transfer_code==1&&!h->transfer_residual;
            }*reason=2;return false;
        }"""+source[last:]
        bad=Path(tmp)/'old.c';bad_exe=Path(tmp)/'old';bad.write_text(PREFIX+old+SUFFIX)
        subprocess.run([*flags,str(bad),'-o',str(bad_exe)],check=True)
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        result=subprocess.run([str(bad_exe)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode==-signal.SIGABRT and b'run(2400000,1000,0,1,0' in result.stderr,result.stderr
    print('PASS: actual production bulk wait/poll ASan/UBSan; PIT frozen, delayed DMA beyond old2M budget succeeds before1s;14bit MFINDEX wrap; actual1s timeout; stalled-clock bound; error/short/residual rejected; old CPU-budget negative control fails same delayed completion')
if __name__=='__main__':main()
