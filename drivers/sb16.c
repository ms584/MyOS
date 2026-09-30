#include "audio.h"
#include "boot.h"

/* ISA DMA channel 1, IRQ 5, I/O 0x220. Never place DMA inside kernel/BSS:
 * the old 0x70000 buffer overwrote graphics state as the runtime grew. */
#define DMA_BUFFER ((uint8_t *)(uintptr_t)boot_state.dma)
#define DMA_BYTES MYOS_SB16_DMA_BYTES
#define HALF 8192u
#define QUEUE_BYTES 65536u
#define DSP 0x220u

static uint8_t queue[QUEUE_BYTES];
static size_t head,tail,used;
static uint64_t irq_count,underrun_count;
static unsigned volume=100,completed_half;
static bool present,running,mute;

static bool dsp_write(uint8_t value){
    for(unsigned i=0;i<100000;++i)if(!(inb(DSP+0x0c)&0x80)){outb(DSP+0x0c,value);return true;}
    return false;
}
static void refill(unsigned half){
    uint8_t *out=DMA_BUFFER+half*HALF;
    bool gap=false;
    for(unsigned i=0;i<HALF;++i){
        uint8_t sample=128;
        if(used){sample=queue[tail];tail=(tail+1)%QUEUE_BYTES;--used;}
        else gap=true;
        int scaled=((int)sample-128)*(int)volume/100;
        out[i]=mute?128:(uint8_t)(scaled+128);
    }
    if(gap)++underrun_count;
}
static void dma_start(void){
    uintptr_t address=(uintptr_t)boot_state.dma;
    outb(0x0a,5);           /* mask channel 1 */
    outb(0x0c,0);           /* reset address/count flip-flop */
    outb(0x02,address&255);outb(0x02,(address>>8)&255);
    outb(0x03,(DMA_BYTES-1)&255);outb(0x03,(DMA_BYTES-1)>>8);
    outb(0x83,address>>16); /* address page */
    outb(0x0b,0x59);        /* single, auto-init, memory-to-device, channel 1 */
    outb(0x0a,1);           /* unmask */
}
bool audio_init(void){
    uint64_t address=boot_state.dma,stack=boot_state.stack;
    /* Validate before programming hardware or touching DMA, including a loader
       ABI mismatch. Firmware allocation owns this range on UEFI. */
    if(address<MYOS_KERNEL_LIMIT||address>0x1000000u-DMA_BYTES||
       (address&0xffffu)+DMA_BYTES>0x10000u||stack<MYOS_INITIAL_STACK_BYTES||
       (address<stack&&address+DMA_BYTES>stack-MYOS_INITIAL_STACK_BYTES))return false;
    outb(DSP+6,1);
    for(volatile unsigned i=0;i<10000;++i)__asm__ volatile("pause");
    outb(DSP+6,0);
    bool reset=false;
    for(unsigned i=0;i<100000;++i)if(inb(DSP+0x0e)&0x80){reset=inb(DSP+0x0a)==0xaa;break;}
    if(!reset)return false;
    if(!dsp_write(0xe1))return false;
    uint8_t major=0,minor=0;
    for(unsigned i=0;i<100000;++i)if(inb(DSP+0x0e)&0x80){major=inb(DSP+0x0a);break;}
    for(unsigned i=0;i<100000;++i)if(inb(DSP+0x0e)&0x80){minor=inb(DSP+0x0a);break;}
    (void)minor;
    if(major<4)return false;
    /* Reject a differently configured card; we program only IRQ5/DMA1. */
    outb(DSP+4,0x80);if(!(inb(DSP+5)&2))return false;
    outb(DSP+4,0x81);if(!(inb(DSP+5)&2))return false;
    present=true;
    outb(0x21,inb(0x21)&~(1u<<5));
    return true;
}
bool audio_ready(void){return present;}
bool audio_queue(const uint8_t *pcm,size_t count){
    if(!present||!pcm||!count||count>QUEUE_BYTES)return false;
    uint64_t flags=irq_save();
    if(count>QUEUE_BYTES-used){irq_restore(flags);return false;}
    for(size_t i=0;i<count;++i){queue[head]=pcm[i];head=(head+1)%QUEUE_BYTES;}
    used+=count;
    if(!running){
        refill(0);refill(1);completed_half=0;
        dma_start();
        bool ok=dsp_write(0x41)&&dsp_write(11025>>8)&&dsp_write(11025&255)&&
                dsp_write(0xd1)&&dsp_write(0xc6)&&dsp_write(0)&&
                dsp_write((HALF-1)&255)&&dsp_write((HALF-1)>>8);
        if(!ok){outb(0x0a,5);irq_restore(flags);return false;}
        running=true;
    }
    irq_restore(flags);return true;
}
void audio_irq(void){
    if(!present)return;
    (void)inb(DSP+0x0e); /* acknowledge 8-bit DMA interrupt */
    ++irq_count;
    if(running){refill(completed_half);completed_half^=1;}
}
void audio_stop(void){
    uint64_t flags=irq_save();
    if(present&&running){(void)dsp_write(0xd0);outb(0x0a,5);(void)inb(DSP+0x0e);}
    running=false;used=head=tail=0;
    irq_restore(flags);
}
void audio_set_volume(unsigned percent){if(percent>100)percent=100;uint64_t f=irq_save();volume=percent;irq_restore(f);}
unsigned audio_volume(void){return volume;}
void audio_set_mute(bool on){uint64_t f=irq_save();mute=on;irq_restore(f);}
bool audio_muted(void){return mute;}
uint64_t audio_underruns(void){return underrun_count;}
uint64_t audio_interrupts(void){return irq_count;}
size_t audio_queued(void){return used;}
