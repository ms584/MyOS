#include "usb.h"
#include "boot.h"
#include "mouse.h"

/* xHCI 1.x: root-attached USB boot mice and BOT storage, polling interrupter 0.
 * DMA remains owned for the controller lifetime, including failed devices.
 * No hubs, hotplug enumeration, UAS, streams or MSI.
 * Spec: Intel xHCI 1.2b sections 4/5/6; USB HID Appendix B boot mouse. */
#define HOSTS 2u
#define SLOTS 8u
#define ENTRIES 256u
#define TYPE(n) ((uint32_t)(n)<<10)
struct trb {uint32_t lo,hi,status,control;};
struct ring {struct trb *data;unsigned index,cycle;};
struct device {
    struct host *host;
    uintptr_t output,input,buffer,mouse_buffer,bulk_buffer;
    struct ring control,interrupt,bulk_in,bulk_out;
    unsigned slot,port,ep,packet,in_ep,out_ep;
    uintptr_t mouse_td;
    bool mouse;
};
struct host {
    uintptr_t base,op,doorbell,interrupter,dcbaa,events,erst;
    struct ring command;
    unsigned event_index,event_cycle,context,ports;
    volatile uintptr_t done_command,done_transfer;
    volatile unsigned command_code,command_slot,transfer_code,transfer_residual;
    struct device devices[SLOTS];
    bool running;
    unsigned init_stage;
    struct {uint32_t before,scan;unsigned stage;} ports_diag[32];
};
static struct host hosts[HOSTS];
static unsigned host_count;
/* Read-only diagnostic counters used by the focused QEMU test. */
static volatile uint64_t usb_mouse_reports,usb_ring_wraps,usb_disconnects;

static uint32_t read32(uintptr_t p){return *(volatile uint32_t *)p;}
static void write32(uintptr_t p,uint32_t v){*(volatile uint32_t *)p=v;}
static void write64(uintptr_t p,uint64_t v){write32(p,(uint32_t)v);write32(p+4,v>>32);}
static uint32_t pci(unsigned b,unsigned d,unsigned f,unsigned r){
    uint32_t a=0x80000000u|(b<<16)|(d<<11)|(f<<8)|(r&0xfc),v;
    __asm__ volatile("outl %0,%1"::"a"(a),"d"((uint16_t)0xcf8));
    __asm__ volatile("inl %1,%0":"=a"(v):"d"((uint16_t)0xcfc));return v;
}
static void pci_command(unsigned b,unsigned d,unsigned f,uint16_t value){
    uint32_t a=0x80000004u|(b<<16)|(d<<11)|(f<<8);
    __asm__ volatile("outl %0,%1"::"a"(a),"d"((uint16_t)0xcf8));
    uint16_t port=0xcfc;
    __asm__ volatile("outw %0,%1"::"a"(value),"d"(port));
}
static uintptr_t page(void){uintptr_t p=page_alloc();if(p)memset((void *)p,0,PAGE_SIZE);return p;}
static bool new_ring(struct ring *r){
    r->data=(void *)page();r->cycle=1;
    if(!r->data)return false;
    r->data[ENTRIES-1]=(struct trb){(uint32_t)(uintptr_t)r->data,0,0,TYPE(6)|2|1};
    return true;
}
static uintptr_t push(struct ring *r,uint64_t pointer,uint32_t status,uint32_t control){
    struct trb *t=&r->data[r->index++];
    t->lo=(uint32_t)pointer;t->hi=pointer>>32;t->status=status;
    __asm__ volatile("":::"memory");
    *(volatile uint32_t *)&t->control=control|r->cycle;
    if(r->index==ENTRIES-1){
        __asm__ volatile("":::"memory");
        *(volatile uint32_t *)&r->data[ENTRIES-1].control=TYPE(6)|2|r->cycle;
        r->index=0;r->cycle^=1;++usb_ring_wraps;
    }
    return (uintptr_t)t;
}
static bool wait_bits(uintptr_t p,uint32_t mask,uint32_t value,unsigned timeout){
    uint64_t end=ticks+timeout;
    do {if((read32(p)&mask)==value)return true;__asm__ volatile("pause");}while(ticks<end);
    return false;
}
static uint32_t port_value(uintptr_t p){
    /* Never echo RW1C PED or change bits when setting power/reset. */
    return (read32(p)&((3u<<14)|(7u<<25)))|(1u<<9);
}
static void submit_mouse(struct host *h,struct device *d){
    memset((void *)d->mouse_buffer,0,64);
    d->mouse_td=push(&d->interrupt,d->mouse_buffer,d->packet,TYPE(1)|(1u<<5)|(1u<<2));
    __asm__ volatile("mfence":::"memory");
    write32(h->doorbell+4*d->slot,d->ep);
}
static void poll(struct host *h){
    for(unsigned budget=0;budget<32;++budget){
        volatile struct trb *t=(void *)(h->events+h->event_index*sizeof(*t));
        uint32_t control=t->control;
        if((control&1)!=h->event_cycle)break;
        __asm__ volatile("lfence":::"memory");
        uint64_t pointer=(uint64_t)t->lo|((uint64_t)t->hi<<32);
        unsigned code=t->status>>24,type=(control>>10)&63,slot=control>>24,ep=(control>>16)&31;
        uint32_t residual=t->status&0xffffffu;
        if(++h->event_index==ENTRIES){h->event_index=0;h->event_cycle^=1;}
        if(type==33){
            h->command_code=code;h->command_slot=slot;h->done_command=pointer;
        }else if(type==32){
            bool handled=false;
            for(unsigned i=0;i<SLOTS;++i){
                struct device *d=&h->devices[i];
                if(!d->mouse||slot!=d->slot||ep!=d->ep)continue;
                handled=true;
                uintptr_t p=h->op+0x400+16*(d->port-1);
                if(!(read32(p)&1)){d->mouse=false;++usb_disconnects;mouse_usb_report(0,0,0);break;}
                if(pointer!=d->mouse_td||residual>d->packet||(code!=1&&code!=13)||d->packet-residual<3){
                    d->mouse=false;mouse_usb_report(0,0,0);print("USB: mouse transfer stopped\n");break;
                }
                uint8_t *report=(void *)d->mouse_buffer;
                mouse_usb_report(report[0]&7,(int8_t)report[1],(int8_t)report[2]);
                ++usb_mouse_reports;submit_mouse(h,d);break;
            }
            if(!handled){h->transfer_code=code;h->transfer_residual=residual;h->done_transfer=pointer;}
        }
        write64(h->interrupter+0x18,(h->events+h->event_index*sizeof(*t))|8u);
    }
    /* Disconnect may occur with an interrupt IN TD still waiting for input. */
    for(unsigned i=0;i<SLOTS;++i){
        struct device *d=&h->devices[i];
        if(d->mouse&&!(read32(h->op+0x400+16*(d->port-1))&1)){
            d->mouse=false;++usb_disconnects;mouse_usb_report(0,0,0);
        }
    }
}
static void stop_host(struct host *h){
    h->running=false;if(h->op)write32(h->op,0);
    for(unsigned i=0;i<SLOTS;++i)if(h->devices[i].mouse){
        h->devices[i].mouse=false;mouse_usb_report(0,0,0);
    }
}
void usb_tick(void){for(unsigned i=0;i<host_count;++i)if(hosts[i].running)poll(&hosts[i]);}
static bool command(struct host *h,uint64_t pointer,uint32_t control,unsigned *slot){
    uint64_t f=irq_save();h->done_command=0;
    uintptr_t expected=push(&h->command,pointer,0,control);
    __asm__ volatile("mfence":::"memory");write32(h->doorbell,0);irq_restore(f);
    uint64_t end=ticks+100;
    do{
        f=irq_save();poll(h);bool done=h->done_command==expected;
        unsigned code=h->command_code;if(slot)*slot=h->command_slot;irq_restore(f);
        if(done)return code==1;
        __asm__ volatile("pause");
    }while(ticks<end);
    print("USB: command timeout\n");stop_host(h);return false;
}
static bool control(struct host *h,struct device *d,uint8_t request_type,uint8_t request,
                    uint16_t value,uint16_t index,unsigned length){
    if(length>1024)return false;
    uint64_t setup=request_type|((uint64_t)request<<8)|((uint64_t)value<<16)|
        ((uint64_t)index<<32)|((uint64_t)length<<48);
    bool input=request_type&0x80;
    if(length&&input)memset((void *)d->buffer,0,length);
    uint64_t f=irq_save();h->done_transfer=0;
    push(&d->control,setup,8,TYPE(2)|(1u<<6)|(length?(input?3u:2u)<<16:0));
    if(length)push(&d->control,d->buffer,length,TYPE(3)|(input?1u<<16:0));
    uintptr_t expected=push(&d->control,0,0,TYPE(4)|(1u<<5)|(!length||!input?1u<<16:0));
    __asm__ volatile("mfence":::"memory");write32(h->doorbell+4*d->slot,1);irq_restore(f);
    uint64_t end=ticks+100;
    do{
        f=irq_save();poll(h);bool done=h->done_transfer==expected;
        unsigned code=h->transfer_code;irq_restore(f);
        if(done)return code==1;
        __asm__ volatile("pause");
    }while(ticks<end);
    print("USB: control timeout\n");stop_host(h);return false;
}
bool usb_bulk_transfer(void *handle,bool input,void *buffer,unsigned length){
    struct device *d=handle;
    if(!d||!buffer||!length||length>512||!d->bulk_buffer)return false;
    struct host *h=d->host;
    if(!h->running||!(read32(h->op+0x400+16*(d->port-1))&1))return false;
    uint64_t flags=irq_save();
    if(input)memset((void *)d->bulk_buffer,0,length);
    else memcpy((void *)d->bulk_buffer,buffer,length);
    h->done_transfer=0;
    uintptr_t expected=push(input?&d->bulk_in:&d->bulk_out,d->bulk_buffer,length,TYPE(1)|(1u<<5)|(1u<<2));
    __asm__ volatile("mfence":::"memory");
    write32(h->doorbell+4*d->slot,input?d->in_ep:d->out_ep);
    uint64_t end=ticks+100;
    /* FS snapshot operations can hold IF=0. Iteration bound still expires
       when PIT ticks cannot advance; never enable IRQs inside FS atomicity. */
    bool ok=false;
    for(unsigned attempts=0;attempts<2000000&&ticks<end;++attempts){
        poll(h);
        if(h->done_transfer==expected){
            ok=h->transfer_code==1&&!h->transfer_residual;break;
        }
        __asm__ volatile("pause");
    }
    if(ok&&input)memcpy(buffer,(void *)d->bulk_buffer,length);
    if(!ok){
        /* No ring/buffer reuse after an uncertain DMA or BOT phase. Reboot
           is required; do not pretend command failures completed writes. */
        stop_host(h);
        print("USB: bulk transport stopped; reboot required\n");
    }
    irq_restore(flags);return ok;
}
static uint32_t *input_context(struct host *h,struct device *d,unsigned n){
    return (void *)(d->input+(uintptr_t)n*h->context);
}
static void enumerate(struct host *h,unsigned port,unsigned speed){
    unsigned *stage=&h->ports_diag[port-1].stage;
    *stage=3;
    if(speed<1||speed>4){print("USB: unsupported port speed\n");return;}
    unsigned index;
    for(index=0;index<SLOTS;++index)if(!h->devices[index].slot)break;
    if(index==SLOTS)return;
    struct device *d=&h->devices[index];
    unsigned slot=0;
    *stage=4;
    if(!command(h,0,TYPE(9),&slot)||!slot||slot>SLOTS)return;
    d->slot=slot;d->port=port;d->host=h;
    *stage=5;
    d->output=page();d->input=page();d->buffer=page();
    if(!d->output||!d->input||!d->buffer||!new_ring(&d->control))return;
    ((uint64_t *)h->dcbaa)[slot]=d->output;
    uint32_t *icc=input_context(h,d,0),*sc=input_context(h,d,1),*ec=input_context(h,d,2);
    icc[1]=3;sc[0]=(speed<<20)|(1u<<27);sc[1]=port<<16;
    unsigned packet=speed==4?512:speed==3?64:8;
    ec[1]=(3u<<1)|(4u<<3)|(packet<<16);
    ec[2]=(uint32_t)(uintptr_t)d->control.data|1;ec[4]=8;
    *stage=6;
    if(!command(h,d->input,TYPE(11)|(slot<<24),NULL))return;
    *stage=7;
    if(!control(h,d,0x80,6,0x100,0,8))return;
    uint8_t *b=(void *)d->buffer;
    if(b[0]!=18||b[1]!=1||!b[7])return;
    unsigned descriptor_packet=speed==4?(b[7]==9?512:0):b[7];
    if(!descriptor_packet)return;
    if(descriptor_packet!=packet){
        packet=descriptor_packet;
        if(packet!=8&&packet!=16&&packet!=32&&packet!=64)return;
        memset((void *)d->input,0,PAGE_SIZE);
        icc[1]=2;memcpy(ec,(void *)(d->output+h->context),h->context);
        ec[1]=(ec[1]&0xffffu)|(packet<<16);
        *stage=8;
        if(!command(h,d->input,TYPE(13)|(slot<<24),NULL))return;
    }
    *stage=9;
    if(!control(h,d,0x80,6,0x100,0,18))return;
    unsigned vendor=b[8]|(b[9]<<8),product=b[10]|(b[11]<<8);
    print("USB device ");hex(vendor);print(":");hex(product);print("\n");
    *stage=10;
    if(!control(h,d,0x80,6,0x200,0,9)||b[0]!=9||b[1]!=2)return;
    unsigned total=b[2]|(b[3]<<8),configuration=b[5];
    *stage=11;
    if(total<9||total>1024||!configuration||!control(h,d,0x80,6,0x200,0,total))return;
    *stage=12;
    unsigned interface=0,endpoint=0,interval=0,mps=0,kind=0;
    unsigned in_ep=0,out_ep=0,in_mps=0,out_mps=0,in_burst=0,out_burst=0,last_ep=0;
    for(unsigned at=0;at<total;){
        unsigned n=b[at];
        if(n<2||at+n>total)return;
        if(b[at+1]==4){
            if(n<9)return;
            if(endpoint||(in_ep&&out_ep))break;
            kind=0;in_ep=out_ep=last_ep=0;in_burst=out_burst=0;
            interface=b[at+2];
            if(!b[at+3]){
                if(b[at+5]==3&&b[at+6]==1&&b[at+7]==2&&speed<=3)kind=1;
                /* xHCI speed1=Full,2=Low; bulk is forbidden at Low speed. */
                if(b[at+5]==8&&b[at+6]==6&&b[at+7]==0x50&&speed!=2)kind=2;
            }
        }else if(b[at+1]==5&&kind){
            if(n<7)return;
            unsigned address=b[at+2],size=b[at+4]|(b[at+5]<<8);
            last_ep=0;
            if((address&0x70)||!(address&15))return;
            unsigned dci=(address&15)*2+!!(address&0x80);
            if(kind==1&&(address&0x80)&&(b[at+3]&3)==3&&size>=3&&size<=64&&b[at+6]){
                endpoint=dci;interval=b[at+6];mps=size;break;
            }
            if(kind==2&&(b[at+3]&3)==2){
                if(speed==4?size!=1024:speed==3?size!=512:(size!=8&&size!=16&&size!=32&&size!=64))return;
                if(address&0x80){if(in_ep)return;in_ep=dci;in_mps=size;}
                else{if(out_ep)return;out_ep=dci;out_mps=size;}
                last_ep=dci;
            }
        }else if(b[at+1]==48&&kind==2&&last_ep&&speed==4){
            if(n<6||b[at+2]>15||b[at+3])return; /* No bulk streams. */
            if(last_ep==in_ep)in_burst=b[at+2];else out_burst=b[at+2];
        }
        at+=n;
    }
    if(!endpoint&&!(in_ep&&out_ep)){print("USB: no supported boot mouse/BOT interface\n");return;}
    *stage=13;
    if(!control(h,d,0,9,configuration,0,0))return;
    if(in_ep&&out_ep){
        *stage=14;
        d->bulk_buffer=page();d->in_ep=in_ep;d->out_ep=out_ep;
        if(!d->bulk_buffer||!new_ring(&d->bulk_in)||!new_ring(&d->bulk_out))return;
        memset((void *)d->input,0,PAGE_SIZE);
        icc[1]=1u|(1u<<in_ep)|(1u<<out_ep);
        memcpy(sc,(void *)d->output,h->context);
        sc[0]=(sc[0]&~(31u<<27))|((in_ep>out_ep?in_ep:out_ep)<<27);
        for(unsigned i=0;i<2;++i){
            unsigned ep=i?out_ep:in_ep,size=i?out_mps:in_mps,burst=i?out_burst:in_burst;
            struct ring *ring=i?&d->bulk_out:&d->bulk_in;
            ec=input_context(h,d,ep+1);
            ec[1]=(3u<<1)|((i?2u:6u)<<3)|(burst<<8)|(size<<16);
            ec[2]=(uint32_t)(uintptr_t)ring->data|1;ec[4]=512;
        }
        if(!command(h,d->input,TYPE(12)|(slot<<24),NULL))return;
        *stage=15;usb_storage_attach(d);return;
    }
    *stage=16;
    if(!control(h,d,0x21,11,0,interface,0))return;
    *stage=17;
    d->mouse_buffer=page();
    if(!d->mouse_buffer||!new_ring(&d->interrupt))return;
    memset((void *)d->input,0,PAGE_SIZE);icc[1]=1u|(1u<<endpoint);
    memcpy(sc,(void *)d->output,h->context);
    sc[0]=(sc[0]&~(31u<<27))|(endpoint<<27);
    unsigned exponent=0;
    if(speed==3){if(interval>16)return;exponent=interval-1;}
    else {for(unsigned n=interval;n>1;n>>=1)++exponent;exponent+=3;}
    ec=input_context(h,d,endpoint+1);
    ec[0]=exponent<<16;ec[1]=(3u<<1)|(7u<<3)|(mps<<16);
    ec[2]=(uint32_t)(uintptr_t)d->interrupt.data|1;ec[4]=mps|(mps<<16);
    *stage=18;
    if(!command(h,d->input,TYPE(12)|(slot<<24),NULL))return;
    *stage=19;
    uint64_t f=irq_save();
    d->ep=endpoint;d->packet=mps;d->mouse=true;
    mouse_usb_attach();submit_mouse(h,d);irq_restore(f);
    print("USB: xHCI HID boot mouse initialized\n");
}
static bool reset_port(uintptr_t reg,unsigned speed){
    if(speed>=4){
        if(read32(reg)&2)return true; /* HCRST already resets USB3 ports. */
        write32(reg,port_value(reg)|(1u<<31));
        /* WPR is a trigger that always reads0. PR reflects reset progress
           for both hot and warm reset (xHCI1.2b5.4.8 bit31). */
    }else write32(reg,port_value(reg)|(1u<<4));
    return wait_bits(reg,1u<<4,0,100)&&(read32(reg)&2);
}
/* HCRST initially disconnects root ports (xHCI1.22 section4.19.4).
 * Discover asynchronous reconnects for2s, requiring100ms stable CCS. One shared
 * deadline covers all ports, including empty ones; no runtime hotplug/retries.
 * Observe only: never echo PORTSC change bits or force power/reset here. */
static uint32_t discover_ports(struct host *h){
    uint64_t since[32]={0},start=ticks;
    uint32_t seen=0,ready=0;
    for(unsigned attempts=0;attempts<2000000&&h->running;++attempts){
        uint64_t now=ticks;
        for(unsigned p=0;p<h->ports;++p){
            uint32_t bit=1u<<p,value=read32(h->op+0x400+16*p);
            h->ports_diag[p].scan=value;
            if(!(value&1)){seen&=~bit;ready&=~bit;continue;}
            if(!(seen&bit)){seen|=bit;since[p]=now;}
            if(now-since[p]>=10)ready|=bit;
        }
        if(now-start>=200)break;
        __asm__ volatile("pause");
    }
    return ready;
}
static bool initialize(struct host *h,unsigned bus,unsigned dev,unsigned fn){
    h->init_stage=1;
    uint32_t bar=pci(bus,dev,fn,0x10);
    if((bar&1)||((bar&6)!=0&&(bar&6)!=4))return false;
    uint64_t physical=bar&~15u;
    if((bar&6)==4)physical|=(uint64_t)pci(bus,dev,fn,0x14)<<32;
    pci_command(bus,dev,fn,(uint16_t)pci(bus,dev,fn,4)|6u);
    h->base=vm_map_mmio(physical,0x10000);
    if(!h->base)return false;
    h->init_stage=2;
    unsigned cap=read32(h->base)&255;
    if(cap<0x20||cap>0x80)return false;
    uint32_t params=read32(h->base+4),hcc=read32(h->base+0x10);
    h->ports=params>>24;h->context=hcc&4?64:32;
    unsigned door=read32(h->base+0x14)&~3u,runtime=read32(h->base+0x18)&~31u;
    if(!h->ports||h->ports>32||(params&255)<SLOTS||door>0xf000||runtime>0xf000)return false;
    h->op=h->base+cap;h->doorbell=h->base+door;h->interrupter=h->base+runtime+0x20;
    for(unsigned p=0;p<h->ports;++p)h->ports_diag[p].before=read32(h->op+0x400+16*p);
    h->init_stage=3;
    /* Bounded legacy ownership capability walk before controller takeover. */
    unsigned offset=(hcc>>16)*4;
    for(unsigned n=0;offset&&n<64;++n){
        if(offset<0x20||offset>0xfff8)return false;
        uint32_t value=read32(h->base+offset);unsigned next=((value>>8)&255)*4;
        if((value&255)==1){
            write32(h->base+offset,value|(1u<<24));
            if(!wait_bits(h->base+offset,1u<<16,0,100))return false;
            write32(h->base+offset+4,0); /* No legacy USB SMI enables. */
        }
        if(!next){offset=0;break;}offset+=next;
    }
    if(offset)return false;
    h->init_stage=4;
    write32(h->op,0);
    if(!wait_bits(h->op+4,1,1,100))return false;
    h->init_stage=5;
    write32(h->op,2);
    if(!wait_bits(h->op,2,0,100)||!wait_bits(h->op+4,1u<<11,0,100)||!(read32(h->op+8)&1))return false;
    h->init_stage=6;
    uint32_t p2=read32(h->base+8);
    unsigned scratch=((p2>>27)&31)|(((p2>>21)&31)<<5);
    if(scratch>16)return false;
    h->dcbaa=page();h->events=page();h->erst=page();
    if(!h->dcbaa||!h->events||!h->erst||!new_ring(&h->command))return false;
    if(scratch){
        uintptr_t array=page();if(!array)return false;
        ((uint64_t *)h->dcbaa)[0]=array;
        for(unsigned i=0;i<scratch;++i){uintptr_t p=page();if(!p)return false;((uint64_t *)array)[i]=p;}
    }
    ((uint64_t *)h->erst)[0]=h->events;((uint32_t *)h->erst)[2]=ENTRIES;
    h->event_cycle=1;
    write64(h->op+0x30,h->dcbaa);write64(h->op+0x18,(uintptr_t)h->command.data|1u);
    write32(h->op+0x38,SLOTS);
    write32(h->interrupter,0);write32(h->interrupter+8,1);
    write64(h->interrupter+0x10,h->erst);write64(h->interrupter+0x18,h->events);
    h->init_stage=7;
    write32(h->op,1);
    if(!wait_bits(h->op+4,1,0,100))return false;
    h->init_stage=8;
    h->running=true;
    print("USB: waiting for root-port reconnect\n");
    uint32_t connected=discover_ports(h);
    for(unsigned p=1;p<=h->ports&&h->running;++p){
        uintptr_t reg=h->op+0x400+16*(p-1);uint32_t value=read32(reg);
        h->ports_diag[p-1].scan=value;
        if(!(value&1))continue;
        if(!(connected&(1u<<(p-1)))){h->ports_diag[p-1].stage=20;continue;}
        h->ports_diag[p-1].stage=1;
        unsigned speed=(value>>10)&15;
        if(!reset_port(reg,speed)){h->ports_diag[p-1].stage=2;continue;}
        uint64_t until=ticks+2;while(ticks<until)__asm__ volatile("pause");
        write32(reg,port_value(reg)|(0x7fu<<17));
        enumerate(h,p,(read32(reg)>>10)&15);
    }
    h->init_stage=9;
    return h->running;
}
void usb_init(void){
    for(unsigned b=0;b<256&&host_count<HOSTS;++b)for(unsigned d=0;d<32&&host_count<HOSTS;++d){
        if((pci(b,d,0,0)&0xffff)==0xffff)continue;
        unsigned functions=(pci(b,d,0,0xc)&0x800000)?8:1;
        for(unsigned f=0;f<functions&&host_count<HOSTS;++f){
            if((pci(b,d,f,8)>>8)!=0x0c0330)continue;
            struct host *h=&hosts[host_count++];
            print("USB: xHCI controller ");hex(pci(b,d,f,0));print("\n");
            if(!initialize(h,b,d,f)){stop_host(h);print("USB: xHCI unavailable; bounded init stopped\n");}
        }
    }
}

/* Observation only: no register acknowledgements, reset, allocation or retries.
 * Snapshots describe the boot attempt; now/last CC can change while polling. */
void usb_status(int selected){
    print("USB hosts=");decimal(host_count);print(" reports=");decimal(usb_mouse_reports);
    print(" wraps=");decimal(usb_ring_wraps);print(" disconnects=");decimal(usb_disconnects);print("\n");
    if(selected>=0&&(unsigned)selected>=host_count){print("USB host absent\n");return;}
    for(unsigned i=0;i<host_count;++i){
        if(selected>=0&&i!=(unsigned)selected)continue;
        struct host *h=&hosts[i];
        print("H");decimal(i);print(" init=");decimal(h->init_stage);print(" running=");decimal(h->running);
        print(" ports=");decimal(h->ports);print(" last command/transfer CC=");decimal(h->command_code);
        print("/");decimal(h->transfer_code);print("\n");
        if(selected<0||!h->op||h->ports>32)continue;
        print("CMD=");hex(read32(h->op));print(" STS=");hex(read32(h->op+4));print("\n");
        for(unsigned p=0;p<h->ports;++p){
            print("P");decimal(p+1);print(" pre=");hex(h->ports_diag[p].before);
            print(" scan=");hex(h->ports_diag[p].scan);print(" now=");hex(read32(h->op+0x400+16*p));
            print(" s=");decimal(h->ports_diag[p].stage);print("\n");
        }
    }
}
