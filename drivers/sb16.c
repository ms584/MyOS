#include "audio.h"
#include "boot.h"

/* ISA DMA channel 1, IRQ 5, I/O 0x220. Never place DMA inside kernel/BSS:
 * the old 0x70000 buffer overwrote graphics state as the runtime grew. */
#define DMA_BUFFER ((uint8_t *)(uintptr_t)boot_state.dma)
#define DMA_BYTES MYOS_SB16_DMA_BYTES
#define HALF 8192u
#define STREAM_HALF 1024u
_Static_assert(STREAM_HALF*2<=DMA_BYTES,"stream DMA fits reserved buffer");
#define QUEUE_BYTES 65536u
#define DSP 0x220u

/* Software staging only, not ISA DMA storage. Allocate for the driver's
 * lifetime after heap initialization and device validation, before IRQ5 is
 * enabled. Keep the 64KiB capacity without consuming BIOS low-memory BSS. */
static uint8_t *queue;
static size_t head,tail,used;
static uint64_t irq_count,underrun_count;
static unsigned volume=100,completed_half;
static bool present,running,mute;
static uint64_t stream_owner,consumed_frames,reported_frames;
static unsigned dma_frames[2];
/* Native Starboard streams must start within Cobalt's local write horizon.
 * Keep the established app audio DMA profile; a leased stream programs a
 * 2048-byte ring (two1024-byte halves,~93ms each at11025Hz). */
static unsigned active_half=HALF,active_dma_bytes=DMA_BYTES;

static bool dsp_write(uint8_t value){
    for(unsigned i=0;i<100000;++i)if(!(inb(DSP+0x0c)&0x80)){outb(DSP+0x0c,value);return true;}
    return false;
}
static void refill(unsigned half){
    uint8_t *out=DMA_BUFFER+half*active_half;
    bool gap=false;
    unsigned frames=0;
    for(unsigned i=0;i<active_half;++i){
        uint8_t sample=128;
        if(used){sample=queue[tail];tail=(tail+1)%QUEUE_BYTES;--used;++frames;}
        else gap=true;
        int scaled=((int)sample-128)*(int)volume/100;
        out[i]=mute?128:(uint8_t)(scaled+128);
    }
    if(gap)++underrun_count;
    dma_frames[half]=frames;
}
static void dma_start(void){
    uintptr_t address=(uintptr_t)boot_state.dma;
    outb(0x0a,5);           /* mask channel 1 */
    outb(0x0c,0);           /* reset address/count flip-flop */
    outb(0x02,address&255);outb(0x02,(address>>8)&255);
    outb(0x03,(active_dma_bytes-1)&255);outb(0x03,(active_dma_bytes-1)>>8);
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
    if(!queue){
        queue=kmalloc(QUEUE_BYTES);
        if(!queue)return false;
        memset(queue,0,QUEUE_BYTES);
    }
    present=true;
    outb(0x21,inb(0x21)&~(1u<<5));
    return true;
}
bool audio_ready(void){return present;}
static bool start_internal(void){
    if(running)return true;
    if(!used)return false;
    size_t saved_tail=tail,saved_used=used;
    refill(0);refill(1);completed_half=0;
    /* A stopped stream's pending IRQ must not complete a new lease's half. */
    (void)inb(DSP+0x0e);
    dma_start();
    bool ok=dsp_write(0x41)&&dsp_write(11025>>8)&&dsp_write(11025&255)&&
            dsp_write(0xd1)&&dsp_write(0xc6)&&dsp_write(0)&&
            dsp_write((active_half-1)&255)&&dsp_write((active_half-1)>>8);
    if(!ok){outb(0x0a,5);tail=saved_tail;used=saved_used;
        dma_frames[0]=dma_frames[1]=0;return false;}
    running=true;return true;
}
static bool queue_internal(const uint8_t *pcm,size_t count,bool start_now){
    if(!present||!pcm||!count||count>QUEUE_BYTES)return false;
    uint64_t flags=irq_save();
    if(count>QUEUE_BYTES-used){irq_restore(flags);return false;}
    size_t saved_head=head,saved_tail=tail,saved_used=used;
    for(size_t i=0;i<count;++i){queue[head]=pcm[i];head=(head+1)%QUEUE_BYTES;}
    used+=count;
    if(!running&&start_now){
        if(!start_internal()){head=saved_head;tail=saved_tail;used=saved_used;
            dma_frames[0]=dma_frames[1]=0;irq_restore(flags);return false;}
    }
    irq_restore(flags);return true;
}
void audio_irq(void){
    if(!present)return;
    outb(DSP+4,0x82);
    if(!(inb(DSP+5)&1))return; /* stale PIC/spurious IRQ, not this DMA block */
    (void)inb(DSP+0x0e); /* acknowledge 8-bit DMA interrupt */
    ++irq_count;
    if(running){consumed_frames+=dma_frames[completed_half];
        refill(completed_half);completed_half^=1;}
}
/* QEMU10.2.1 i8257 current-count register is base count minus DMA cursor.
 * Mask DMA1 during the two byte reads to prevent rollover/torn snapshots.
 * IRQs are already disabled by callers; a pending half-boundary is included
 * once even when the PIC handler has not run yet. */
static uint64_t played_snapshot(bool restart){
    uint64_t played=consumed_frames;
    if(running){
        outb(0x0a,5);outb(0x0c,0);
        unsigned remaining=inb(0x03);remaining|=(unsigned)inb(0x03)<<8;
        if(remaining<active_dma_bytes){
            unsigned cursor=active_dma_bytes-1-remaining,half=cursor/active_half,partial=cursor%active_half;
            if(half!=completed_half)played+=dma_frames[completed_half];
            if(partial>dma_frames[half])partial=dma_frames[half];
            played+=partial;
        }
        if(restart)outb(0x0a,1);
    }
    if(played<reported_frames)played=reported_frames;
    reported_frames=played;return played;
}
static void stop_internal(void){
    uint64_t flags=irq_save();
    if(present&&running){(void)dsp_write(0xd0);
        consumed_frames=played_snapshot(false);(void)inb(DSP+0x0e);}
    running=false;used=head=tail=0;
    dma_frames[0]=dma_frames[1]=0;
    irq_restore(flags);
}
/* Native PCM device lease: callers use a validated full process identity.
 * All state transitions serialize with DMA IRQs. No user pointers live here. */
bool audio_queue(const uint8_t *pcm,size_t count){
    uint64_t flags=irq_save();bool ok=!stream_owner&&queue_internal(pcm,count,true);
    irq_restore(flags);return ok;
}
void audio_stop(void){uint64_t flags=irq_save();if(!stream_owner)stop_internal();irq_restore(flags);}
int audio_stream_acquire(uint64_t owner){
    uint64_t flags=irq_save();int result=0;
    if(!owner)result=-22;
    else if(!present)result=-19;
    else if(stream_owner||used||dma_frames[0]||dma_frames[1])result=-16;
    else {stop_internal();active_half=HALF;active_dma_bytes=DMA_BYTES;
        stream_owner=owner;consumed_frames=reported_frames=0;}
    irq_restore(flags);return result;
}
int audio_stream_queue(uint64_t owner,const uint8_t *pcm,size_t count){
    uint64_t flags=irq_save();int result;
    if(!owner||owner!=stream_owner)result=-9;
    else if(!pcm||!count||count>QUEUE_BYTES)result=-22;
    else result=queue_internal(pcm,count,used+count>=active_dma_bytes)?0:-11;
    irq_restore(flags);return result;
}
int audio_stream_status(uint64_t owner,struct audio_stream_status *status){
    uint64_t flags=irq_save();int result=0;
    if(!owner||owner!=stream_owner)result=-9;
    else if(!status)result=-22;
    else {
        uint64_t played=played_snapshot(true),pending=dma_frames[0]+dma_frames[1];
        uint64_t partial=played-consumed_frames;
        pending=partial<pending?pending-partial:0;
        *status=(struct audio_stream_status){played,(uint32_t)used,
            (uint32_t)pending,QUEUE_BYTES,11025,1,8};
    }
    irq_restore(flags);return result;
}
void audio_stream_release(uint64_t owner){
    uint64_t flags=irq_save();if(owner&&owner==stream_owner){stop_internal();stream_owner=0;active_half=HALF;active_dma_bytes=DMA_BYTES;
        consumed_frames=reported_frames=0;}
    irq_restore(flags);
}
int audio_stream_flush(uint64_t owner){
    uint64_t flags=irq_save();int result=-9;
    if(owner&&owner==stream_owner){stop_internal();result=0;}
    irq_restore(flags);return result;
}
int audio_stream_start(uint64_t owner){
    uint64_t flags=irq_save();int result=-9;
    if(owner&&owner==stream_owner)result=start_internal()?0:-11;
    irq_restore(flags);return result;
}
int audio_stream_configure(uint64_t owner,unsigned half_frames){
    uint64_t flags=irq_save();int result=0;
    if(!owner||owner!=stream_owner)result=-9;
    else if(half_frames!=STREAM_HALF&&half_frames!=HALF)result=-22;
    else if(running||used||dma_frames[0]||dma_frames[1])result=-16;
    else {active_half=half_frames;active_dma_bytes=half_frames*2;}
    irq_restore(flags);return result;
}
void audio_set_volume(unsigned percent){if(percent>100)percent=100;uint64_t f=irq_save();volume=percent;irq_restore(f);}
unsigned audio_volume(void){return volume;}
void audio_set_mute(bool on){uint64_t f=irq_save();mute=on;irq_restore(f);}
bool audio_muted(void){return mute;}
uint64_t audio_underruns(void){return underrun_count;}
uint64_t audio_interrupts(void){return irq_count;}
size_t audio_queued(void){return used;}
