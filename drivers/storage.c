#include "fs-snapshot.h"

#define STORAGE_BANK_SECTORS (1u+FS_NODES*FS_SNAPSHOT_SECTORS_PER_SLOT)
#define STORAGE_BANK_A 16u
#define STORAGE_BANK_B (STORAGE_BANK_A+STORAGE_BANK_SECTORS)
#define STORAGE_END (STORAGE_BANK_B+STORAGE_BANK_SECTORS)
#define STORAGE_VERSION 1u
#define STORAGE_MAGIC 0x315346534f594dull /* "MYOSFS1" in little endian. */

extern void ata_init(void);
extern bool ata_read(uint32_t,void *);
extern bool ata_write(uint32_t,const void *);
extern bool ata_flush(void);

struct header {
    uint64_t magic;
    uint32_t version,sectors;
    uint64_t generation;
    uint32_t checksum;
    uint8_t reserved[STORAGE_SECTOR_SIZE-28];
};
_Static_assert(sizeof(struct header)==STORAGE_SECTOR_SIZE,"header sector size");
_Static_assert(sizeof(struct fs_disk_slot)==FS_SNAPSHOT_SECTORS_PER_SLOT*STORAGE_SECTOR_SIZE,"snapshot slot size");
static uint64_t generation;
static int active_bank=-1;
static bool ready;
static const char disk_signature[]="MYOS-DATA-v1";

static uint32_t checksum(const void *buffer,size_t size)
{
    const uint8_t *data=buffer;uint32_t state=2166136261u;
    for(size_t i=0;i<size;++i){state^=data[i];state*=16777619u;}
    return state;
}
static uint32_t bank_start(unsigned bank){return bank?STORAGE_BANK_B:STORAGE_BANK_A;}
static bool read_slots(unsigned bank,struct fs_disk_slot *slots)
{
    uint8_t *bytes=(uint8_t *)slots;uint32_t start=bank_start(bank)+1;
    for(unsigned i=0;i<FS_NODES*FS_SNAPSHOT_SECTORS_PER_SLOT;++i)
        if(!ata_read(start+i,bytes+(size_t)i*STORAGE_SECTOR_SIZE))return false;
    return true;
}
static bool write_slots(unsigned bank,const struct fs_disk_slot *slots)
{
    const uint8_t *bytes=(const uint8_t *)slots;uint32_t start=bank_start(bank)+1;
    for(unsigned i=0;i<FS_NODES*FS_SNAPSHOT_SECTORS_PER_SLOT;++i)
        if(!ata_write(start+i,bytes+(size_t)i*STORAGE_SECTOR_SIZE))return false;
    return true;
}
static bool valid_header(const struct header *h)
{
    return h->magic==STORAGE_MAGIC && h->version==STORAGE_VERSION
           && h->sectors==STORAGE_BANK_SECTORS && h->generation>0;
}
void storage_init(void)
{
    ata_init();
    if(storage_present() && storage_sectors()<STORAGE_END){
        print("Storage: disk smaller than two snapshot banks; RAM-only\n");
        return;
    }
    if(storage_present()){
        uint8_t sector[STORAGE_SECTOR_SIZE];
        if(!ata_read(0,sector)){
            print("Storage: could not read data disk signature; RAM-only\n");return;
        }
        for(unsigned i=0;i<sizeof(disk_signature);++i)
            if(sector[i]!=(uint8_t)disk_signature[i]){
                print("Storage: unrecognized data disk; RAM-only\n");return;
            }
        ready=true;
    }
}
bool storage_ready(void){return ready;}
int storage_load(void)
{
    if(!ready)return -19;
    struct fs_disk_slot *slots=kmalloc(sizeof(*slots)*FS_NODES);
    if(!slots)return -12;
    int found=-1;uint64_t newest=0;bool invalid=false;
    for(unsigned bank=0;bank<2;++bank){
        struct header h;
        if(!ata_read(bank_start(bank),&h)){invalid=true;continue;}
        if(!valid_header(&h)){
            if(h.magic==STORAGE_MAGIC)invalid=true;
            continue;
        }
        if(!read_slots(bank,slots)){invalid=true;continue;}
        if(checksum(slots,sizeof(*slots)*FS_NODES)!=h.checksum ||
           !fs_snapshot_valid(slots)){invalid=true;continue;}
        if(h.generation>newest){newest=h.generation;found=(int)bank;}
    }
    if(found<0){kfree(slots);return invalid?-5:-2;}
    if(!read_slots((unsigned)found,slots)){kfree(slots);return -5;}
    struct header h;
    if(!ata_read(bank_start((unsigned)found),&h) ||
       checksum(slots,sizeof(*slots)*FS_NODES)!=h.checksum ||
       !fs_snapshot_import(slots)){kfree(slots);return -5;}
    kfree(slots);active_bank=found;generation=newest;
    return 0;
}
int storage_save(void)
{
    if(!ready)return -19;
    if(generation==UINT64_MAX)return -28;
    struct fs_disk_slot *slots=kmalloc(sizeof(*slots)*FS_NODES);
    if(!slots)return -12;
    if(!fs_snapshot_export(slots)){kfree(slots);return -16;}
    unsigned bank=active_bank==0?1:0;
    struct header h={0};h.magic=STORAGE_MAGIC;h.version=STORAGE_VERSION;
    h.sectors=STORAGE_BANK_SECTORS;h.generation=generation+1;
    h.checksum=checksum(slots,sizeof(*slots)*FS_NODES);
    bool ok=write_slots(bank,slots)&&ata_flush()&&ata_write(bank_start(bank),&h)&&ata_flush();
    kfree(slots);
    if(!ok)return -5;
    generation=h.generation;active_bank=(int)bank;return 0;
}
uint64_t storage_generation(void){return generation;}
