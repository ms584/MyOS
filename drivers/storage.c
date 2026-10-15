#include "fs-snapshot.h"
#include "block.h"
#define STORAGE_BANK_A 16u
#define STORAGE_MAGIC 0x315346534f594dull /* MYOSFS1, unchanged across versions. */
struct header {
    uint64_t magic;
    uint32_t version,sectors;
    uint64_t generation;
    uint32_t checksum;
    uint8_t reserved[STORAGE_SECTOR_SIZE-28];
};
_Static_assert(sizeof(struct header)==STORAGE_SECTOR_SIZE,"header sector size");
static uint64_t generation;
static int active_bank=-1;
static bool ready;
static unsigned version,slot_bytes,bank_sectors;
static size_t payload_bytes;
static uint32_t checksum(const void *buffer,size_t size)
{
    const uint8_t *data=buffer;uint32_t state=2166136261u;
    for(size_t i=0;i<size;++i){state^=data[i];state*=16777619u;}return state;
}
static uint32_t bank_start(unsigned bank){return STORAGE_BANK_A+(bank?bank_sectors:0);}
static bool read_slots(unsigned bank,void *slots)
{
    uint8_t *bytes=slots;uint32_t start=bank_start(bank)+1;
    for(unsigned i=0;i<payload_bytes/STORAGE_SECTOR_SIZE;++i)
        if(!block_read(start+i,bytes+(size_t)i*STORAGE_SECTOR_SIZE))return false;
    return true;
}
static bool write_slots(unsigned bank,const void *slots)
{
    const uint8_t *bytes=slots;uint32_t start=bank_start(bank)+1;
    for(unsigned i=0;i<payload_bytes/STORAGE_SECTOR_SIZE;++i)
        if(!block_write(start+i,bytes+(size_t)i*STORAGE_SECTOR_SIZE))return false;
    return true;
}
static bool valid_header(const struct header *h)
{return h->magic==STORAGE_MAGIC&&h->version==version&&h->sectors==bank_sectors&&h->generation>0;}
void storage_init(void)
{
    block_init();if(!storage_present())return;
    uint8_t sector[STORAGE_SECTOR_SIZE];
    if(!block_read(0,sector)){print("Storage: could not read data disk signature; RAM-only\n");return;}
    static const char v1[]="MYOS-DATA-v1",v2[]="MYOS-DATA-v2";
    const char *signature=0;
    bool one=true,two=true;
    for(unsigned i=0;i<sizeof(v1);++i){if(sector[i]!=(uint8_t)v1[i])one=false;if(sector[i]!=(uint8_t)v2[i])two=false;}
    if(one){signature=v1;version=1;slot_bytes=FS_V1_SLOT_BYTES;}
    if(two){signature=v2;version=2;slot_bytes=FS_V2_SLOT_BYTES;}
    if(!signature){print("Storage: unrecognized data disk; RAM-only\n");return;}
    payload_bytes=(size_t)slot_bytes*FS_NODES;
    bank_sectors=1+(unsigned)(payload_bytes/STORAGE_SECTOR_SIZE);
    if(storage_sectors()<bank_start(1)+bank_sectors){
        print("Storage: disk smaller than two snapshot banks; RAM-only\n");return;
    }
    ready=true;
}
bool storage_ready(void){return ready;}
int storage_load(void)
{
    if(!ready)return -19;
    void *slots=kmalloc(payload_bytes);if(!slots)return -12;
    int found=-1;uint64_t newest=0;bool invalid=false;
    for(unsigned bank=0;bank<2;++bank){
        struct header h;
        if(!block_read(bank_start(bank),&h)){invalid=true;continue;}
        if(!valid_header(&h)){if(h.magic==STORAGE_MAGIC)invalid=true;continue;}
        if(!read_slots(bank,slots)){invalid=true;continue;}
        if(checksum(slots,payload_bytes)!=h.checksum||
           !fs_snapshot_valid(slots,slot_bytes,slot_bytes-STORAGE_SECTOR_SIZE)){invalid=true;continue;}
        if(h.generation>newest){newest=h.generation;found=(int)bank;}
    }
    if(found<0){kfree(slots);return invalid?-5:-2;}
    if(!read_slots((unsigned)found,slots)){kfree(slots);return -5;}
    struct header h;
    if(!block_read(bank_start((unsigned)found),&h)||checksum(slots,payload_bytes)!=h.checksum||
       !fs_snapshot_import(slots,slot_bytes,slot_bytes-STORAGE_SECTOR_SIZE)){kfree(slots);return -5;}
    kfree(slots);active_bank=found;generation=newest;return 0;
}
int storage_save(void)
{
    if(!ready)return -19;if(generation==UINT64_MAX)return -28;
    void *slots=kmalloc(payload_bytes);if(!slots)return -12;
    if(!fs_snapshot_export(slots,slot_bytes,slot_bytes-STORAGE_SECTOR_SIZE)){kfree(slots);return -28;}
    unsigned bank=active_bank==0?1:0;
    struct header h={0};h.magic=STORAGE_MAGIC;h.version=version;
    h.sectors=bank_sectors;h.generation=generation+1;h.checksum=checksum(slots,payload_bytes);
    bool ok=write_slots(bank,slots)&&block_flush()&&block_write(bank_start(bank),&h)&&block_flush();
    kfree(slots);if(!ok)return -5;
    generation=h.generation;active_bank=(int)bank;return 0;
}
uint64_t storage_generation(void){return generation;}
