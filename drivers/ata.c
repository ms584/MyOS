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
static uint32_t asset_capacity;
static uint8_t asset_drive;
static uint8_t selected_drive; /* 0 = primary master, 1 = primary slave. */
static const char data_signature_v1[]="MYOS-DATA-v1";
static const char data_signature_v2[]="MYOS-DATA-v2";

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
    outb(ATA_DRIVE,(uint8_t)(0xe0|(selected_drive<<4)|(lba>>24)));
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
        if(ok){void *destination=buffer;size_t words=256;
            /* One sector, still PIO and still under the same IRQ exclusion.
             * String I/O permits the hypervisor to batch the transfer instead
             * of handling 256 independent IN instructions. No DMA or change
             * to sector boundaries, status checks or write authorization. */
            __asm__ volatile("cld; rep insw"
                : "+D"(destination), "+c"(words) : "d"((uint16_t)ATA_DATA)
                : "memory", "cc");
            delay_400ns();
        }
    }
    irq_restore(flags);return ok;
}
bool ata_write(uint32_t lba,const void *buffer)
{
    if(!detected || lba>=capacity || !buffer || (asset_capacity&&selected_drive==asset_drive))return false;
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
    if(ok){outb(ATA_DRIVE,(uint8_t)(0xe0|(selected_drive<<4)));outb(ATA_COMMAND,0xe7);ok=wait_status(false);}
    irq_restore(flags);return ok;
}
static bool identify(unsigned drive,uint32_t *sectors)
{
    outb(ATA_DRIVE,(uint8_t)(0xa0|(drive<<4)));delay_400ns();
    outb(ATA_COUNT,0);outb(ATA_LBA0,0);outb(ATA_LBA1,0);outb(ATA_LBA2,0);
    outb(ATA_COMMAND,0xec);
    uint8_t status=inb(ATA_STATUS);
    if(!status || status==0xff || !wait_status(true) ||
       inb(ATA_LBA1) || inb(ATA_LBA2))return false;
    uint16_t words[256];
    for(unsigned i=0;i<256;++i)words[i]=inw(ATA_DATA);
    uint32_t count=(uint32_t)words[60]|((uint32_t)words[61]<<16);
    if(!(words[49]&(1u<<9)) || count<2048 || count>0x0fffffff)return false;
    *sectors=count;return true;
}
static bool is_data_disk(void)
{
    uint8_t sector[512];
    if(!ata_read(0,sector))return false;
    bool v1=true,v2=true;
    for(unsigned i=0;i<sizeof(data_signature_v1);++i){
        if(sector[i]!=(uint8_t)data_signature_v1[i])v1=false;
        if(sector[i]!=(uint8_t)data_signature_v2[i])v2=false;
    }
    return v1||v2;
}
void ata_init(void)
{
    /* Prefer a marked data disk on either primary IDE device. A BIOS HDD
       occupies the master position; a separate data disk can use the slave. */
    outb(ATA_CONTROL,2); /* Disable ATA IRQ; use bounded polling. */
    /* Discover the separate asset volume first, without authorizing writes. */
    asset_capacity=0;
    for(unsigned drive=0;drive<2;++drive){
        uint32_t sectors=0;if(!identify(drive,&sectors))continue;
        selected_drive=(uint8_t)drive;capacity=sectors;detected=true;
        uint8_t sector[512];static const char magic[16]="MYOS-ASSET-v1",magic2[16]="MYOS-ASSET-v2";
        if(!ata_read(0,sector))continue;
        bool match=true,match2=true;for(unsigned i=0;i<16;++i){
            if(sector[i]!=(uint8_t)magic[i])match=false;
            if(sector[i]!=(uint8_t)magic2[i])match2=false;}
        if((match||match2)&&!asset_capacity){asset_drive=(uint8_t)drive;asset_capacity=sectors;}
    }
    bool fallback=false;uint32_t fallback_capacity=0;uint8_t fallback_drive=0;
    for(unsigned drive=0;drive<2;++drive){
        uint32_t sectors=0;
        if(!identify(drive,&sectors))continue;
        if(!fallback){fallback=true;fallback_capacity=sectors;fallback_drive=(uint8_t)drive;}
        selected_drive=(uint8_t)drive;capacity=sectors;detected=true;
        if(is_data_disk()){
            print("ATA PIO disk: ");decimal(sectors);print(" sectors\n");return;
        }
    }
    if(fallback){selected_drive=fallback_drive;capacity=fallback_capacity;detected=true;
        outb(ATA_DRIVE,(uint8_t)(0xa0|(fallback_drive<<4)));delay_400ns();
        print("ATA PIO disk: ");decimal(capacity);print(" sectors\n");
    }else{detected=false;capacity=0;print("ATA: no data disk; RAM-only files\n");}
}
bool ata_present(void){return detected;}
uint32_t ata_sectors(void){return capacity;}

uint32_t ata_asset_sectors(void){return asset_capacity;}
bool ata_asset_read(uint32_t sector,void *buffer)
{
    if(!asset_capacity||sector>=asset_capacity)return false;
    uint64_t flags=irq_save();
    bool saved_detected=detected;uint32_t saved_capacity=capacity;uint8_t saved_drive=selected_drive;
    detected=true;capacity=asset_capacity;selected_drive=asset_drive;
    outb(ATA_DRIVE,(uint8_t)(0xe0|(asset_drive<<4)));delay_400ns();
    bool ok=ata_read(sector,buffer);
    detected=saved_detected;capacity=saved_capacity;selected_drive=saved_drive;
    outb(ATA_DRIVE,(uint8_t)(0xe0|(saved_drive<<4)));delay_400ns();
    irq_restore(flags);return ok;
}
