#include "block.h"
#include "usb.h"
extern void ata_init(void);
extern bool ata_present(void);
extern uint32_t ata_sectors(void);
extern bool ata_read(uint32_t,void *);
extern bool ata_write(uint32_t,const void *);
extern bool ata_flush(void);
static bool use_usb;
void block_init(void){
    ata_init();use_usb=false;
    /* Preserve legacy marked ATA snapshots. Prefer the explicitly bounded USB
       partition over an unknown IDE boot/asset disk. No system-NVMe access. */
    uint8_t sector[512];bool marked=false;
    if(ata_present()&&ata_read(0,sector)){
        static const char prefix[]="MYOS-DATA-v";
        marked=true;
        for(unsigned i=0;i<sizeof(prefix)-1;++i)if(sector[i]!=(uint8_t)prefix[i])marked=false;
        if((sector[11]!='1'&&sector[11]!='2')||sector[12])marked=false;
    }
    use_usb=!marked&&usb_storage_present();
}
const char *storage_backend(void){return use_usb?"USB BOT":"ATA PIO";}
bool storage_present(void){return use_usb?usb_storage_present():ata_present();}
uint32_t storage_sectors(void){return use_usb?usb_storage_sectors():ata_sectors();}
bool block_read(uint32_t lba,void *p){return use_usb?usb_storage_read(lba,p):ata_read(lba,p);}
bool block_write(uint32_t lba,const void *p){return use_usb?usb_storage_write(lba,p):ata_write(lba,p);}
bool block_flush(void){return use_usb?usb_storage_flush():ata_flush();}
