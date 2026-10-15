#include "usb.h"
/* USB MSC Bulk-Only Transport 1.0; one LUN and one 512B block per command.
 * Only the prepared MyOS partition is exposed. No GPT, UAS, arbitrary disk
 * writes, reset recovery or silent flush emulation. Fail closed until reboot. */
#define DATA_LBA 69632u
#define DATA_SECTORS 32768u
struct cbw {
    uint32_t magic,tag,length;
    uint8_t flags,lun,cdb_length,cdb[16];
} __attribute__((packed));
struct csw {uint32_t magic,tag,residue;uint8_t status;} __attribute__((packed));
_Static_assert(sizeof(struct cbw)==31&&sizeof(struct csw)==13,"BOT wrapper sizes");
static void *medium;
static bool live,ready;
static uint32_t next_tag,capacity;
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static uint32_t le32(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put_be32(uint8_t *p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int scsi(const uint8_t *cdb,unsigned cdb_length,bool input,void *data,unsigned length){
    if(!live||!cdb_length||cdb_length>16||length>512||(length&&!data))return -1;
    uint64_t flags=irq_save();
    struct cbw cb={0};struct csw cs={0};
    cb.magic=0x43425355;cb.tag=++next_tag;cb.length=length;
    cb.flags=input?0x80:0;cb.cdb_length=cdb_length;memcpy(cb.cdb,cdb,cdb_length);
    bool ok=usb_bulk_transfer(medium,false,&cb,sizeof(cb))&&
        (!length||usb_bulk_transfer(medium,input,data,length))&&
        usb_bulk_transfer(medium,true,&cs,sizeof(cs));
    int result=-1;
    if(ok&&cs.magic==0x53425355&&cs.tag==cb.tag&&!cs.residue&&cs.status<=1)result=cs.status;
    else{live=false;ready=false;print("USB BOT: invalid/failed transport; storage disabled\n");}
    irq_restore(flags);return result;
}
static bool read_raw(uint32_t lba,void *buffer){
    if(lba>=capacity)return false;
    uint8_t cdb[10]={0x28};put_be32(cdb+2,lba);cdb[8]=1;
    return scsi(cdb,10,true,buffer,512)==0;
}
void usb_storage_attach(void *handle){
    if(ready)return;
    medium=handle;live=true;capacity=0;
    uint8_t cdb[10]={0x12,0,0,0,36},data[512]={0};
    if(scsi(cdb,6,true,data,36)!=0||(data[0]&31)!=0)goto rejected;
    memset(cdb,0,sizeof(cdb));
    bool available=false;
    for(unsigned attempt=0;attempt<3&&live;++attempt){
        if(scsi(cdb,6,true,NULL,0)==0){available=true;break;}
        uint8_t sense[6]={3,0,0,0,18,0};
        if(scsi(sense,6,true,data,18)!=0)break;
    }
    if(!available)goto rejected;
    cdb[0]=0x25;
    if(scsi(cdb,10,true,data,8)!=0||be32(data+4)!=512||be32(data)==0xffffffffu)goto rejected;
    capacity=be32(data)+1;
    if(capacity<DATA_LBA+DATA_SECTORS||!read_raw(0,data))goto rejected;
    /* Match exact bounded MBR layout; reject extra entries, GPT/protective
       MBR and overlaps. The size may be the whole owner's 32GB USB. */
    if(data[510]!=0x55||data[511]!=0xaa||data[446]!=0x80||data[450]!=0xef||
       le32(data+454)!=2048||le32(data+458)!=65536||
       data[462]||data[466]!=0xda||le32(data+470)!=DATA_LBA||le32(data+474)!=DATA_SECTORS)goto rejected;
    for(unsigned i=478;i<510;++i)if(data[i])goto rejected;
    if(!read_raw(DATA_LBA,data))goto rejected;
    static const char signature[]="MYOS-DATA-v2";
    for(unsigned i=0;i<sizeof(signature);++i)if(data[i]!=(uint8_t)signature[i])goto rejected;
    memset(cdb,0,sizeof(cdb));cdb[0]=0x35; /* Mandatory SYNCHRONIZE CACHE(10). */
    if(scsi(cdb,10,false,NULL,0)!=0)goto rejected;
    ready=true;
    print("USB BOT: prepared MyOS data partition, 32768 sectors (512B)\n");return;
rejected:
    ready=false;live=false;medium=NULL;capacity=0;
    print("USB BOT: no supported prepared data partition; RAM-only\n");
}
bool usb_storage_present(void){return ready&&live;}
uint32_t usb_storage_sectors(void){return usb_storage_present()?DATA_SECTORS:0;}
bool usb_storage_read(uint32_t lba,void *buffer){
    return usb_storage_present()&&lba<DATA_SECTORS&&buffer&&read_raw(DATA_LBA+lba,buffer);
}
bool usb_storage_write(uint32_t lba,const void *buffer){
    /* Even a caller bug cannot write MBR, ESP or the immutable data marker. */
    if(!usb_storage_present()||!lba||lba>=DATA_SECTORS||!buffer)return false;
    uint8_t cdb[10]={0x2a};put_be32(cdb+2,DATA_LBA+lba);cdb[8]=1;
    return scsi(cdb,10,false,(void *)buffer,512)==0;
}
bool usb_storage_flush(void){
    uint8_t cdb[10]={0x35};
    return usb_storage_present()&&scsi(cdb,10,false,NULL,0)==0;
}
