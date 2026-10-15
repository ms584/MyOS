"""Run production xHCI reset/stop logic against asynchronous MMIO registers.
Only read32/write32 are adapted; privileged/unused driver sections are discarded.
The WPR register always reads zero while PR remains set until reset completes.
"""
from pathlib import Path
import subprocess,tempfile,resource,signal
ROOT=Path(__file__).resolve().parents[1]
PREFIX=r"""
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
static uint32_t state;
static unsigned reads,writes,releases,stops;
static bool pending,never_complete;
volatile uint64_t ticks;
static uint32_t test_read(uintptr_t address){
    assert(address==0x1400);++ticks;
    if(pending&&++reads==4&&!never_complete){state=(state&~16u)|2u;pending=false;}
    return state&~(1u<<31);
}
static void test_write(uintptr_t address,uint32_t value){
    if(address==0x1000){assert(!value);++stops;return;}
    assert(address==0x1400&&!(value&2)); /* Never echo PED/RW1C. */
    assert(value&(1u<<9));
    assert(value&(16u|(1u<<31)));
    ++writes;state=value|16u|1u;pending=true;reads=0;
}
"""
SUFFIX=r"""
void mouse_usb_report(uint8_t buttons,int dx,int dy){
    assert(!buttons&&!dx&&!dy);++releases;
}
static void setup(unsigned speed,bool enabled){
    state=1|(speed<<10)|(1u<<9)|(enabled?2u:0);
    reads=writes=0;pending=never_complete=false;ticks=0;
}
int main(void){
    setup(4,false);
    assert(reset_port(0x1400,4));
    assert(writes==1&&reads==4&&(state&2)); /* Old WPR wait returns too early. */
    setup(4,true);
    assert(reset_port(0x1400,4)&&!writes); /* HCRST completed automatic USB3 reset. */
    setup(1,false);
    assert(reset_port(0x1400,1)&&reads==4&&writes==1);
    setup(4,false);never_complete=true;
    assert(!reset_port(0x1400,4)&&reads>=99&&reads<110); /* Finite deadline. */
    struct host h={0};h.op=0x1000;h.running=true;
    h.devices[0].mouse=true;h.devices[0].mouse_buffer=0xdeadbeef;
    stop_host(&h);
    assert(!h.running&&!h.devices[0].mouse&&releases==1&&stops==1);
    assert(h.devices[0].mouse_buffer==0xdeadbeef); /* DMA ownership retained. */
    stop_host(&h);assert(releases==1); /* No repeated phantom release. */
    return 0;
}
"""
def main():
    source=(ROOT/'drivers/xhci.c').read_text()
    old_read='static uint32_t read32(uintptr_t p){return *(volatile uint32_t *)p;}'
    old_write='static void write32(uintptr_t p,uint32_t v){*(volatile uint32_t *)p=v;}'
    assert source.count(old_read)==1 and source.count(old_write)==1
    source=source.replace(old_read,'static uint32_t read32(uintptr_t p){return test_read(p);}')
    source=source.replace(old_write,'static void write32(uintptr_t p,uint32_t v){test_write(p,v);}')
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='xhci-ports-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test'
        c.write_text(PREFIX+source+SUFFIX)
        subprocess.run(['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror',
                        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
                        '-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(c),'-o',str(exe)],check=True)
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        subprocess.run([str(exe)],check=True)
        bad=c.with_name('old-wpr.c');bad_exe=c.with_name('old-wpr')
        old=source.replace('return wait_bits(reg,1u<<4,0,100)','return wait_bits(reg,1u<<31,0,100)')
        assert old!=source
        bad.write_text(PREFIX+old+SUFFIX)
        subprocess.run(['gcc','-std=c11','-O1','-fno-builtin','-ffunction-sections',
                        '-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),
                        str(bad),'-o',str(bad_exe)],check=True)
        rejected=subprocess.run([str(bad_exe)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert rejected.returncode==-signal.SIGABRT and b'reset_port' in rejected.stderr
        print('PASS: same asynchronous-port regression rejects the old WPR polling defect')
    print('PASS: production xHCI warm reset waits on PR while WPR reads0; USB2 reset; already-enabled USB3; bounded timeout; failed-controller button release and DMA ownership')
if __name__=='__main__':main()
