"""Read-only production USB report against bounded MMIO fixtures, ASan/UBSan."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
PREFIX=r"""
#include <assert.h>
#include <string.h>
#include <stdint.h>
static unsigned mmio_reads,mmio_writes;
static char output[16384];
static unsigned used;
static uint32_t test_read(uintptr_t p){
    ++mmio_reads;
    if(p==0x2000||p==0x2004||p==0x2400)return 0;
    if(p==0x1000)return 1;
    if(p==0x1004)return 0;
    assert(p==0x1400||p==0x1410);
    return p==0x1400?0x1203:0x200;
}
static void test_write(uintptr_t p,uint32_t v){(void)p;(void)v;++mmio_writes;}
"""
SUFFIX=r"""
void print(const char *s){while(*s){assert(used+1<sizeof output);output[used++]=*s++;}output[used]=0;}
void decimal(uint64_t n){char b[22];unsigned i=0;do{b[i++]='0'+n%10;n/=10;}while(n);while(i){char c[2]={b[--i],0};print(c);}}
void hex(uint64_t n){char b[19]="0x0000000000000000";for(int i=17;i>=2;--i){b[i]="0123456789abcdef"[n&15];n>>=4;}print(b);}
int main(void){
    usb_status(-1);assert(strstr(output,"USB hosts=0"));assert(!mmio_reads);
    used=0;usb_status(0);assert(strstr(output,"USB host absent"));assert(!mmio_reads);
    host_count=1;struct host *h=&hosts[0];h->op=0x1000;h->ports=2;h->running=true;h->init_stage=9;
    h->ports_diag[0].before=0x1201;h->ports_diag[0].scan=0x1203;h->ports_diag[0].stage=19;
    h->ports_diag[1].before=0x200;h->ports_diag[1].scan=0x200;
    h->command_code=1;h->transfer_code=13;usb_mouse_reports=27;
    struct host saved=*h;
    used=0;usb_status(-1);assert(strstr(output,"H0 init=9 running=1 ports=2 last command/transfer CC=1/13"));assert(!mmio_reads);
    used=0;usb_status(0);
    assert(strstr(output,"P1 pre=0x0000000000001201 scan=0x0000000000001203 now=0x0000000000001203 s=19"));
    assert(strstr(output,"P2 pre=0x0000000000000200 scan=0x0000000000000200 now=0x0000000000000200 s=0"));
    assert(mmio_reads==4&&!mmio_writes&&!memcmp(h,&saved,sizeof saved)&&usb_mouse_reports==27);
    char first[sizeof output];memcpy(first,output,sizeof output);
    used=0;usb_status(0);assert(!strcmp(first,output)&&mmio_reads==8&&!mmio_writes);
    used=0;usb_status(1);assert(strstr(output,"USB host absent")&&mmio_reads==8);
    host_count=2;hosts[1].op=0x2000;hosts[1].ports=1;hosts[1].init_stage=5;
    used=0;usb_status(1);assert(strstr(output,"H1 init=5 running=0 ports=1"));
    assert(!strstr(output,"H0 init=")&&mmio_reads==11&&!mmio_writes);
    used=0;usb_status(-1);assert(strstr(output,"H0 init=")&&strstr(output,"H1 init=")&&mmio_reads==11);
    h->op=0;used=0;usb_status(0);assert(mmio_reads==11);
    h->op=0x1000;h->ports=33;used=0;usb_status(0);assert(mmio_reads==11);
    return 0;
}
"""
def main():
    source=(ROOT/'drivers/xhci.c').read_text()
    old='static uint32_t read32(uintptr_t p){return *(volatile uint32_t *)p;}'
    assert source.count(old)==1
    source=source.replace(old,'static uint32_t read32(uintptr_t p){return test_read(p);}')
    old='static void write32(uintptr_t p,uint32_t v){*(volatile uint32_t *)p=v;}'
    assert source.count(old)==1
    source=source.replace(old,'static void write32(uintptr_t p,uint32_t v){test_write(p,v);}')
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='usb-diag-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(PREFIX+source+SUFFIX)
        subprocess.run(['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-fsanitize=address,undefined',
            '-I'+str(ROOT/'include'),str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS: USB production diagnostics ASan/UBSan; exact snapshots/stages; absent/failed/invalid host bounds; repeated reads preserve registers, driver state and counters; zero MMIO writes')
if __name__=='__main__':main()
