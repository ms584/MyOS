"""Regression for physical reset-time CCS=0 followed by delayed root attach.
Native fixtures use the actual driver discovery function; QEMU attaches a mouse
inside the boot discovery window, not after Shell/runtime hotplug.
"""
from pathlib import Path
import subprocess,tempfile,signal,resource,time
ROOT=Path(__file__).resolve().parents[1]
PREFIX=r"""
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
static unsigned reads,writes,ports,mode,elapsed;
static bool frozen;
volatile uint64_t ticks;
static uint32_t test_read(uintptr_t address){
    assert(address>=0x1400&&(address-0x1400)%16==0);
    unsigned p=(address-0x1400)/16;assert(p<ports);
    unsigned t=elapsed;bool connected=false;
    if(mode==0)connected=(p==0&&t>=20)||(p==1&&t>=130)||
        (p==2&&((t>=20&&t<30)||(t>=70&&t<190)))||(p==3&&t>=195)||p==5;
    if(mode==2)connected=t>=195;
    if(mode==3)connected=true;
    if(mode==4)connected=p==31;
    ++reads;if(reads%ports==0&&!frozen){++ticks;++elapsed;}
    return connected?(p==1?0x1203:0x6e1):0x2a0;
}
static void test_write(uintptr_t p,uint32_t value){(void)p;(void)value;++writes;assert(0);}
"""
SUFFIX=r"""
static uint32_t run_case(unsigned count,unsigned kind,bool stall,uint64_t start){
    struct host h={0};h.op=0x1000;h.ports=ports=count;h.running=true;
    reads=writes=elapsed=0;mode=kind;frozen=stall;ticks=start;
    uint32_t mask=discover_ports(&h);assert(!writes);
    if(!stall)assert(elapsed>=200&&elapsed<=202); /* Shared bound, not32 deadlines. */
    return mask;
}
int main(void){
    assert(run_case(6,0,false,0)==((1u<<0)|(1u<<1)|(1u<<5)));
    assert(run_case(6,1,false,0)==0); /* Empty ports terminate. */
    assert(run_case(6,2,false,0)==0); /* Late/not stable100ms is not reset. */
    assert(run_case(1,3,true,0)==0&&reads==2000000); /* PIT failure finite. */
    assert(run_case(32,4,false,0)==(1u<<31)); /* Highest supported root port. */
    assert(run_case(6,0,false,UINT64_MAX-100)==((1u<<0)|(1u<<1)|(1u<<5)));
    struct host stopped={0};reads=0;assert(!discover_ports(&stopped)&&!reads);
    return 0;
}
"""
def native():
    source=(ROOT/'drivers/xhci.c').read_text()
    assert 'uint32_t connected=discover_ports(h);' in source
    read='static uint32_t read32(uintptr_t p){return *(volatile uint32_t *)p;}'
    write='static void write32(uintptr_t p,uint32_t v){*(volatile uint32_t *)p=v;}'
    assert source.count(read)==source.count(write)==1
    source=source.replace(read,'static uint32_t read32(uintptr_t p){return test_read(p);}')
    source=source.replace(write,'static void write32(uintptr_t p,uint32_t v){test_write(p,v);}')
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='usb-startup-') as tmp:
        c=Path(tmp)/'test.c';exe=Path(tmp)/'test';c.write_text(PREFIX+source+SUFFIX)
        flags=['gcc','-std=c11','-O1','-g','-fno-builtin','-Wall','-Wextra','-Werror',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include')]
        subprocess.run([*flags,'-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        start=source.index('static uint32_t discover_ports(')
        end=source.index('\nstatic bool initialize(',start)
        old=source[:start]+"""static uint32_t discover_ports(struct host *h){
            uint32_t ready=0;for(unsigned p=0;p<h->ports;++p)
                if(read32(h->op+0x400+16*p)&1)ready|=1u<<p;
            return ready;
        }"""+source[end:]
        # Same first delayed-attach assertion, without the deadline assertion:
        # isolate missing devices as the negative-control failure.
        old_suffix=SUFFIX.replace('if(!stall)assert(elapsed>=200&&elapsed<=202);','if(!stall)(void)elapsed;')
        bad=Path(tmp)/'old.c';bad_exe=Path(tmp)/'old';bad.write_text(PREFIX+old+old_suffix)
        subprocess.run([*flags,str(bad),'-o',str(bad_exe)],check=True)
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        result=subprocess.run([str(bad_exe)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode==-signal.SIGABRT and b'run_case(6,0,false,0)' in result.stderr,result.stderr
    print('PASS: production startup discovery ASan/UBSan; delayed mouse+SS device, bounce/detach, late/empty ports, shared2s deadline, stable100ms CCS, stalled PIT finite,32ports, tick wrap; old one-shot scan rejected',flush=True)

def delayed_mouse():
    from test_system import VM,symbols
    from test_shell import command
    from test_mouse import send_relative,send_button
    from test_usb_mouse import number,wait_number
    from test_firmware import FIRMWARE
    for name,firmware in [('bios',None),('uefi',FIRMWARE)]:
        vm=VM('usb-startup-delayed-'+name,ram=128,boot_image=ROOT/'build/myos-usb.img',
              boot_bus='ide',firmware=firmware,xhci_devices=[])
        try:
            vm.wait('USB: waiting for root-port reconnect')
            time.sleep(.15)
            assert 'MyOS Shell ready' not in vm.log.read_text(),'fixture missed startup window'
            vm.command('device_add',{'driver':'usb-mouse','bus':'xhci.0','id':'late-mouse'})
            text=vm.wait('myos> ')
            assert 'USB: xHCI HID boot mouse initialized' in text,text
            command(vm,'usb 0','s=19')
            a=symbols();before=number(vm,a,'usb_mouse_reports')
            send_relative(vm,'x',25);send_relative(vm,'y',-12)
            wait_number(vm,a,'usb_mouse_reports',before+1)
            command(vm,'mouse','Mouse: x=425 y=288 buttons=0')
            send_button(vm,True);command(vm,'mouse','buttons=1')
            send_button(vm,False);command(vm,'mouse','buttons=0')
            print('PASS: '+name.upper()+' real QEMU mouse attached150ms after R/S startup; enumerated, motion/buttons and keyboard/Shell work (IDE boot isolates startup fixture)',flush=True)
        finally:vm.close()
if __name__=='__main__':
    import sys
    if '--qemu-only' not in sys.argv:native()
    if '--native-only' not in sys.argv:delayed_mouse()
