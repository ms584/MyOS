#include "asset.h"
#define ASSET_LIMIT 32u
struct volume { char magic[16]; uint32_t version,count; uint64_t bytes; };
struct entry { char name[48]; uint64_t offset,size; };
_Static_assert(sizeof(struct entry)==64,"asset entry layout");
static bool same(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool raw(uint64_t off,void *buffer,size_t n)
{
    uint64_t capacity=(uint64_t)ata_asset_sectors()*512;
    if(off>capacity||n>capacity-off||(!buffer&&n))return false;
    uint8_t sector[512],*dst=buffer;
    while(n){
        size_t skip=off%512,take=512-skip;if(take>n)take=n;
        if(!ata_asset_read((uint32_t)(off/512),sector))return false;
        memcpy(dst,sector+skip,take);dst+=take;off+=take;n-=take;
    }
    return true;
}
static bool table(struct entry entries[ASSET_LIMIT],unsigned *count)
{
    struct volume h;static const char magic[16]="MYOS-ASSET-v1";
    if(!raw(0,&h,sizeof(h)))return false;
    for(unsigned i=0;i<16;++i)if(h.magic[i]!=magic[i])return false;
    if(h.version!=1||!h.count||h.count>ASSET_LIMIT||h.bytes<4096||
       h.bytes>(uint64_t)ata_asset_sectors()*512)return false;
    if(!raw(512,entries,h.count*sizeof(*entries)))return false;
    for(unsigned i=0;i<h.count;++i){
        struct entry *e=&entries[i];bool end=false;
        if(!e->name[0]||e->offset<4096||e->offset%512||!e->size||
           e->offset>h.bytes||e->size>h.bytes-e->offset)return false;
        for(unsigned j=0;j<48;++j){
            unsigned char c=(unsigned char)e->name[j];
            if(!c){end=true;break;}
            if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
        }
        if(!end)return false;
        for(unsigned j=0;j<i;++j)if(same(e->name,entries[j].name)||
          (e->offset<entries[j].offset+entries[j].size&&entries[j].offset<e->offset+e->size))return false;
    }
    *count=h.count;return true;
}
int asset_open(const char *name,struct asset_file *file)
{
    struct entry entries[ASSET_LIMIT];unsigned count;
    if(!name||!file)return -22;
    if(!ata_asset_sectors())return -19;
    if(!table(entries,&count))return -8;
    for(unsigned i=0;i<count;++i)if(same(name,entries[i].name)){
        *file=(struct asset_file){entries[i].offset,entries[i].size};return 0;
    }
    return -2;
}
bool asset_read(void *context,uint64_t off,void *buffer,size_t n)
{
    const struct asset_file *f=context;
    if(!f||off>f->size||n>f->size-off||f->offset>UINT64_MAX-off)return false;
    return raw(f->offset+off,buffer,n);
}
int asset_stat(const char *name,uint64_t *size)
{
    struct asset_file file;int rc=asset_open(name,&file);
    if(!rc&&size)*size=file.size;
    return rc;
}
int asset_entry(unsigned position,char name[48],uint64_t *size)
{
    struct entry entries[ASSET_LIMIT];unsigned count;
    if(!table(entries,&count))return ata_asset_sectors()?-8:-19;
    if(position>=count)return 0;
    memcpy(name,entries[position].name,48);if(size)*size=entries[position].size;return 1;
}
void asset_list(void)
{
    struct entry entries[ASSET_LIMIT];unsigned count;
    if(!table(entries,&count)){print("Assets: unavailable or invalid volume\n");return;}
    print("Assets: read-only\n");
    for(unsigned i=0;i<count;++i){print(entries[i].name);print(" ");decimal(entries[i].size);print(" bytes\n");}
}
