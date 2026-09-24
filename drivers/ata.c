#include "storage.h"

#define ATA_DATA 0x1f0
#define ATA_ERROR 0x1f1
#define ATA_COUNT 0x1f2
#define ATA_LBA0 0x1f3
#define ATA_LBA1 0x1f4
#define ATA_LBA2 0x1f5
#define ATA_DRIVE 0x1f6
#define ATA_STATUS 0x1f7
#define ATA_COMMAND 0x1f7
#define ATA_CONTROL 0x3f6
#define ATA_BSY 0x80
#define ATA_DRQ 0x08
#define ATA_ERR 0x01
#define ATA_DF 0x20

static bool detected;
static uint32_t capacity;

static uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile("inw %1,%0":"=a"(value):"Nd"(port));
    return value;
}
static void outw(uint16_t port,uint16_t value)
{
    __asm__ volatile("outw %0,%1"::"a"(value),"Nd"(port));
}
static void delay_400ns(void)
{
    for(unsigned i=0;i<4;++i)(void)inb(ATA_CONTROL);
}
static bool wait_status(bool data)
{
    /* A finite bound prevents a missing or failed controller hanging MyOS. */
    for(unsigned tries=0;tries<1000000;++tries){
        uint8_t status=inb(ATA_STATUS);
        if(status==0 || status==0xff)return false;
        if(!(status&ATA_BSY)){
            if(status&(ATA_ERR|ATA_DF))return false;
            if(!data || (status&ATA_DRQ))return true;
        }
    }
    return false;
}
static void select_sector(uint32_t lba)
{
    outb(ATA_DRIVE,(uint8_t)(0xe0|(lba>>24)));
    delay_400ns();
    outb(ATA_COUNT,1);
    outb(ATA_LBA0,(uint8_t)lba);
    outb(ATA_LBA1,(uint8_t)(lba>>8));
    outb(ATA_LBA2,(uint8_t)(lba>>16));
}
bool ata_read(uint32_t lba,void *buffer)
{
    if(!detected || lba>=capacity || !buffer)return false;
    uint64_t flags=irq_save();
    bool ok=wait_status(false);
    if(ok){
        select_sector(lba);outb(ATA_COMMAND,0x20);
        ok=wait_status(true);
        if(ok){uint8_t *bytes=buffer;
            for(unsigned i=0;i<256;++i){uint16_t value=inw(ATA_DATA);bytes[2*i]=(uint8_t)value;bytes[2*i+1]=(uint8_t)(value>>8);}
            delay_400ns();
        }
    }
    irq_restore(flags);return ok;
}
bool ata_write(uint32_t lba,const void *buffer)
{
    if(!detected || lba>=capacity || !buffer)return false;
    uint64_t flags=irq_save();
    bool ok=wait_status(false);
    if(ok){
        select_sector(lba);outb(ATA_COMMAND,0x30);
        ok=wait_status(true);
        if(ok){const uint8_t *bytes=buffer;
            for(unsigned i=0;i<256;++i)outw(ATA_DATA,(uint16_t)(bytes[2*i]|((uint16_t)bytes[2*i+1]<<8)));
            delay_400ns();ok=wait_status(false);
        }
    }
    irq_restore(flags);return ok;
}
bool ata_flush(void)
{
    if(!detected)return false;
    uint64_t flags=irq_save();bool ok=wait_status(false);
    if(ok){outb(ATA_DRIVE,0xe0);outb(ATA_COMMAND,0xe7);ok=wait_status(false);}
    irq_restore(flags);return ok;
}
void ata_init(void)
{
    /* Legacy QEMU IDE primary master. BIOS floppy uses a separate controller. */
    outb(ATA_CONTROL,2); /* Disable ATA IRQ; use bounded polling. */
    outb(ATA_DRIVE,0xa0);delay_400ns();
    outb(ATA_COUNT,0);outb(ATA_LBA0,0);outb(ATA_LBA1,0);outb(ATA_LBA2,0);
    outb(ATA_COMMAND,0xec);
    uint8_t status=inb(ATA_STATUS);
    if(!status || status==0xff){print("ATA: no data disk; RAM-only files\n");return;}
    if(!wait_status(true) || inb(ATA_LBA1) || inb(ATA_LBA2)){
        print("ATA: unsupported or unresponsive primary master\n");return;
    }
    uint16_t words[256];
    for(unsigned i=0;i<256;++i)words[i]=inw(ATA_DATA);
    uint32_t sectors=(uint32_t)words[60]|((uint32_t)words[61]<<16);
    if(!(words[49]&(1u<<9)) || sectors<2048 || sectors>0x0fffffff){
        print("ATA: LBA28 disk is too small or unsupported\n");return;
    }
    capacity=sectors;detected=true;
    print("ATA PIO disk: ");decimal(sectors);print(" sectors\n");
}
bool storage_present(void){return detected;}
uint32_t storage_sectors(void){return capacity;}
