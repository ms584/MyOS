#include "asset.h"
#define ASSET_LIMIT 32u
#define V2_LIMIT 1024u
struct volume {char magic[16];uint32_t version,count;uint64_t bytes;};
struct entry {char name[48];uint64_t offset,size;};
struct volume2 {struct volume base;uint64_t runtime_offset,runtime_size;};
struct entry2 {char name[240];uint64_t offset,size;};
_Static_assert(sizeof(struct entry)==64,"asset v1 entry");
_Static_assert(sizeof(struct volume2)==48&&sizeof(struct entry2)==256,"asset v2 layout");
/* Immutable volume metadata belongs to the mount, not an application. No
 * hotplug/remount ABI exists; each boot starts with an empty bounded cache. */
static struct entry2 *catalogue;
static struct volume2 mounted;
static int mount_error;
static size_t length_of(const char *text){size_t n=0;while(text[n])++n;return n;}
static bool same(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int compare(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
static bool starts(const char *text,const char *prefix)
{while(*prefix)if(*text++!=*prefix++)return false;return true;}
static bool raw(uint64_t off,void *buffer,size_t n)
{
    uint64_t capacity=(uint64_t)ata_asset_sectors()*512;
    if(off>capacity||n>capacity-off||(!buffer&&n))return false;
    uint8_t sector[512],*dst=buffer;
    while(n){size_t skip=off%512,take=512-skip;if(take>n)take=n;
        if(!ata_asset_read((uint32_t)(off/512),sector))return false;
        memcpy(dst,sector+skip,take);dst+=take;off+=take;n-=take;}
    return true;
}
static bool name_ok(const char *name,unsigned capacity,bool hierarchy)
{
    unsigned length=0;while(length<capacity&&name[length])++length;
    if(!length||length==capacity)return false;
    unsigned component=0;
    for(unsigned i=0;i<length;++i){unsigned char c=(unsigned char)name[i];
        if(c=='/'&&hierarchy){if(!component)return false;component=0;continue;}
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
        if(++component>(hierarchy?239u:47u))return false;
        /* No '.'/'..' components, including the final component. */
        if(hierarchy&&c=='.'&&(!i||name[i-1]=='/')&&
           (i+1==length||name[i+1]=='/'||(name[i+1]=='.'&&
             (i+2==length||name[i+2]=='/'))))return false;
    }
    return component>0;
}
static bool v1_table(struct entry entries[ASSET_LIMIT],unsigned *count)
{
    struct volume h;static const char magic[16]="MYOS-ASSET-v1";
    if(!raw(0,&h,sizeof(h)))return false;
    for(unsigned i=0;i<16;++i)if(h.magic[i]!=magic[i])return false;
    if(h.version!=1||!h.count||h.count>ASSET_LIMIT||h.bytes<4096||
       h.bytes>(uint64_t)ata_asset_sectors()*512||!raw(512,entries,h.count*sizeof(*entries)))return false;
    for(unsigned i=0;i<h.count;++i){struct entry *e=&entries[i];
        if(!name_ok(e->name,48,false)||e->offset<4096||e->offset%512||!e->size||
           e->offset>h.bytes||e->size>h.bytes-e->offset)return false;
        for(unsigned j=0;j<i;++j)if(same(e->name,entries[j].name)||
          (e->offset<entries[j].offset+entries[j].size&&entries[j].offset<e->offset+e->size))return false;
    }
    *count=h.count;return true;
}
static int version(void)
{struct volume h;if(!raw(0,&h,sizeof(h)))return 0;return (int)h.version;}
static bool v2_mount(void)
{
    if(catalogue)return true;
    mount_error=-8;
    struct volume2 h;static const char magic[16]="MYOS-ASSET-v2";
    if(!raw(0,&h,sizeof(h)))return false;
    for(unsigned i=0;i<16;++i)if(h.base.magic[i]!=magic[i])return false;
    uint64_t data_begin=(512+(uint64_t)h.base.count*sizeof(struct entry2)+511)&~511ull;
    if(data_begin<4096)data_begin=4096;
    if(h.base.version!=2||!h.base.count||h.base.count>V2_LIMIT||h.base.bytes<data_begin||
       h.base.bytes>(uint64_t)ata_asset_sectors()*512)return false;
    if(!!h.runtime_offset!=!!h.runtime_size)return false;
    if(h.runtime_size&&(h.runtime_offset<data_begin||h.runtime_offset%512||
       h.runtime_offset>h.base.bytes||h.runtime_size>h.base.bytes-h.runtime_offset))return false;
    struct entry2 *items=kmalloc(h.base.count*sizeof(*items));if(!items){mount_error=-12;return false;}
    bool valid=raw(512,items,h.base.count*sizeof(*items));
    for(unsigned i=0;valid&&i<h.base.count;++i){struct entry2 *e=&items[i];
        valid=name_ok(e->name,240,true)&&e->size&&e->offset>=data_begin&&! (e->offset%512)&&
            e->offset<=h.base.bytes&&e->size<=h.base.bytes-e->offset;
        if(valid&&i)valid=compare(items[i-1].name,e->name)<0&&
            items[i-1].offset+items[i-1].size<=e->offset;
        if(valid&&h.runtime_size)valid=!(e->offset<h.runtime_offset+h.runtime_size&&
            h.runtime_offset<e->offset+e->size);
        for(unsigned j=0;valid&&j<i;++j){size_t n=length_of(items[j].name);
            if(starts(e->name,items[j].name)&&e->name[n]=='/')valid=false;}
    }
    if(!valid){kfree(items);return false;}
    mounted=h;catalogue=items;return true;
}
static int lookup(const char *name)
{
    unsigned low=0,high=mounted.base.count;
    while(low<high){unsigned middle=low+(high-low)/2;int order=compare(name,catalogue[middle].name);
        if(!order)return (int)middle;if(order<0)high=middle;else low=middle+1;}
    return -2;
}
int asset_open(const char *name,struct asset_file *file)
{
    if(!name||!file)return -22;if(!ata_asset_sectors())return -19;
    if(version()==2){if(!v2_mount())return mount_error;int found=lookup(name);if(found<0)return found;
        *file=(struct asset_file){catalogue[found].offset,catalogue[found].size};return 0;}
    struct entry entries[ASSET_LIMIT];unsigned count;if(!v1_table(entries,&count))return -8;
    for(unsigned i=0;i<count;++i)if(same(name,entries[i].name)){
        *file=(struct asset_file){entries[i].offset,entries[i].size};return 0;}
    return -2;
}
int asset_runtime_open(struct asset_file *file)
{
    if(!file)return -22;if(!ata_asset_sectors())return -19;
    if(version()!=2)return -8;if(!v2_mount())return mount_error;
    if(!mounted.runtime_size)return -2;
    *file=(struct asset_file){mounted.runtime_offset,mounted.runtime_size};return 0;
}
bool asset_read(void *context,uint64_t off,void *buffer,size_t n)
{const struct asset_file *f=context;
 if(!f||off>f->size||n>f->size-off||f->offset>UINT64_MAX-off)return false;
 return raw(f->offset+off,buffer,n);}
int asset_stat(const char *name,uint64_t *size)
{struct asset_file file;int rc=asset_open(name,&file);if(!rc&&size)*size=file.size;return rc;}
int asset_directory(const char *name)
{
    if(!name)return -22;if(version()!=2)return -2;if(!v2_mount())return mount_error;
    if(!*name)return 0;
    size_t length=length_of(name);if(length>=239)return -36;
    for(unsigned i=0;i<mounted.base.count;++i)
        if(starts(catalogue[i].name,name)&&catalogue[i].name[length]=='/')return 0;
    return -2;
}
int asset_child_long(const char *directory,unsigned position,char name[240],uint64_t *size,unsigned *type)
{
    if(!directory||!name||!type)return -22;
    if(version()!=2){if(*directory)return -2;char legacy[48];int rc=asset_entry(position,legacy,size);
        if(rc==1){memset(name,0,240);memcpy(name,legacy,48);*type=1;}return rc;}
    if(!v2_mount())return mount_error;
    int rc=asset_directory(directory);if(rc){if(lookup(directory)>=0)return -20;return rc;}
    size_t prefix=length_of(directory);unsigned seen=0;char previous[240]={0};
    for(unsigned i=0;i<mounted.base.count;++i){const char *full=catalogue[i].name,*tail=full;
        if(prefix){if(!starts(full,directory)||full[prefix]!='/')continue;tail=full+prefix+1;}
        char child[240]={0};unsigned n=0;while(tail[n]&&tail[n]!='/'){child[n]=tail[n];++n;}
        if(same(child,previous))continue;memcpy(previous,child,sizeof(previous));
        if(seen++!=position)continue;
        memcpy(name,child,240);*type=tail[n]=='/'?2:1;if(size)*size=*type==2?0:catalogue[i].size;return 1;
    }
    return 0;
}
int asset_child(const char *directory,unsigned position,char name[48],uint64_t *size,unsigned *type)
{if(!name)return -22;char full[240];int rc=asset_child_long(directory,position,full,size,type);
 if(rc==1){memset(name,0,48);unsigned i=0;while(i<47&&full[i]){name[i]=full[i];++i;}}return rc;}
int asset_entry(unsigned position,char name[48],uint64_t *size)
{
    if(version()==2){unsigned type;return asset_child("",position,name,size,&type);}
    struct entry entries[ASSET_LIMIT];unsigned count;
    if(!v1_table(entries,&count))return ata_asset_sectors()?-8:-19;
    if(position>=count)return 0;memcpy(name,entries[position].name,48);if(size)*size=entries[position].size;return 1;
}
void asset_list(void)
{
    if(version()==2){if(!v2_mount()){print("Assets: unavailable or invalid volume\n");return;}
        print("Assets: read-only\n");for(unsigned i=0;i<mounted.base.count;++i){
            print(catalogue[i].name);print(" ");decimal(catalogue[i].size);print(" bytes\n");}return;}
    struct entry entries[ASSET_LIMIT];unsigned count;
    if(!v1_table(entries,&count)){print("Assets: unavailable or invalid volume\n");return;}
    print("Assets: read-only\n");for(unsigned i=0;i<count;++i){print(entries[i].name);print(" ");decimal(entries[i].size);print(" bytes\n");}
}
