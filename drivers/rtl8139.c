#include "nic.h"
/* RTL8139 in legacy I/O mode: a bounded, polling QEMU driver. */
static uint16_t base;
static uintptr_t receive_page,send_page[4];
static uint8_t address[6];
static unsigned received_offset,send_index;
static bool online;
static uint16_t inw(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static uint32_t inl(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static void outw(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static void outl(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static uint32_t pci_read(unsigned bus,unsigned device,unsigned function,unsigned reg)
{
    outl(0xcf8,0x80000000u|(bus<<16)|(device<<11)|(function<<8)|(reg&0xfc));
    return inl(0xcfc);
}
static void pci_write(unsigned bus,unsigned device,unsigned function,unsigned reg,uint32_t value)
{
    outl(0xcf8,0x80000000u|(bus<<16)|(device<<11)|(function<<8)|(reg&0xfc));
    outl(0xcfc,value);
}
static void release_buffers(void)
{
    for(unsigned i=0;i<4;++i){if(send_page[i]){page_free(send_page[i]);send_page[i]=0;}}
    if(receive_page){for(unsigned i=0;i<4;++i)page_free(receive_page+i*PAGE_SIZE);receive_page=0;}
}
bool nic_init(void)
{
    unsigned bus=0,device=0,function=0;bool found=false;
    for(unsigned b=0;b<256&&!found;++b)for(unsigned d=0;d<32&&!found;++d)
        for(unsigned f=0;f<8;++f)
            if(pci_read(b,d,f,0)==0x813910ecu){bus=b;device=d;function=f;found=true;break;}
    if(!found)return false;
    uint32_t bar=pci_read(bus,device,function,0x10);
    if(!(bar&1)||!((bar&~3u)&0xffffu))return false;
    base=(uint16_t)(bar&~3u);
    uint32_t command=pci_read(bus,device,function,0x04);
    pci_write(bus,device,function,0x04,(command&0xffff0000u)|((command|5u)&0xffffu));
    receive_page=pages_alloc_contiguous(4);
    if(!receive_page)return false;
    for(unsigned i=0;i<4;++i){
        send_page[i]=page_alloc();
        if(!send_page[i]){release_buffers();return false;}
        memset((void *)send_page[i],0,PAGE_SIZE);
    }
    memset((void *)receive_page,0,4*PAGE_SIZE);
    outb(base+0x52,0); /* power on */
    outb(base+0x37,0x10);
    unsigned retry=1000000;
    while((inb(base+0x37)&0x10)&&retry--){}
    if(inb(base+0x37)&0x10){release_buffers();return false;}
    for(unsigned i=0;i<6;++i)address[i]=inb(base+i);
    outw(base+0x3c,0); /* masked IRQ; protocol layer polls. */
    outl(base+0x30,(uint32_t)receive_page);
    outl(base+0x44,0x8f); /* 8 KiB ring, contiguous wrap, accept unicast/broadcast. */
    outb(base+0x37,0x0c); /* RX and TX enable. */
    online=true;
    return true;
}
bool nic_ready(void){return online;}
/* RTL8139 MediaStatus LinkDown bit2; independent of a cached DHCP lease. */
bool nic_link_up(void){return online&&!(inb(base+0x58)&4);}
const uint8_t *nic_mac(void){return address;}
bool nic_send(const void *data,size_t length)
{
    if(!online||!data||length<14||length>NIC_FRAME_MAX)return false;
    unsigned slot=send_index++&3u;
    unsigned retry=1000000;
    while(!(inl(base+0x10+4*slot)&(1u<<13))&&retry--){}
    if(!retry)return false;
    size_t actual=length<60?60:length;
    memcpy((void *)send_page[slot],data,length);
    if(actual>length)memset((void *)(send_page[slot]+length),0,actual-length);
    outl(base+0x20+4*slot,(uint32_t)send_page[slot]);
    outl(base+0x10+4*slot,(uint32_t)actual);
    return true;
}
unsigned nic_poll(nic_receive_fn receive,unsigned budget)
{
    if(!online||!receive)return 0;
    unsigned count=0;
    while(count<budget&&!(inb(base+0x37)&1)){
        uint8_t *entry=(uint8_t *)(receive_page+received_offset);
        uint16_t status=(uint16_t)entry[0]|((uint16_t)entry[1]<<8);
        uint16_t size=(uint16_t)entry[2]|((uint16_t)entry[3]<<8);
        if(!(status&1)||size<18||size>NIC_FRAME_MAX+4){
            outb(base+0x37,0x10);
            unsigned retry=1000000;while((inb(base+0x37)&0x10)&&retry--){}
            outl(base+0x30,(uint32_t)receive_page);
            outl(base+0x44,0x8f);outb(base+0x37,0x0c);
            received_offset=0;break;
        }
        receive(entry+4,size-4);++count;
        received_offset=(received_offset+size+4+3)&~3u;
        received_offset&=8191u;
        outw(base+0x38,(uint16_t)(received_offset-16));
    }
    outw(base+0x3e,inw(base+0x3e));
    return count;
}
