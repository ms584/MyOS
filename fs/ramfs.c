#include "fs-snapshot.h"
#define FS_ASSET_NODE UINT32_MAX
struct node {
    bool used, unlinked;
    unsigned parent, type, references;
    size_t size, capacity;
    char name[FS_NAME];
    char *data;
};
static struct node nodes[FS_NODES];

static bool equal(const char *a,const char *b)
{
    while (*a && *a==*b) { ++a; ++b; }
    return *a==*b;
}
static int child(unsigned parent,const char *name)
{
    for (unsigned i=1;i<FS_NODES;++i)
        if (nodes[i].used && !nodes[i].unlinked && nodes[i].parent==parent && equal(nodes[i].name,name)) return (int)i;
    return -2;
}

/* Absolute paths only. Collapse '/', '.', and '..'; '..' at root stays root. */
static int resolve(const char *path,bool parent,char *leaf)
{
    if (!path || path[0]!='/') return -22;
    size_t length=0;
    while (length<FS_PATH && path[length]) ++length;
    if (length==FS_PATH) return -36;
    unsigned current=0;
    size_t pos=1;
    while (pos<length) {
        while (path[pos]=='/') ++pos;
        if (!path[pos]) break;
        if (nodes[current].type!=FS_DIR) return -20;
        char name[FS_NAME]; size_t size=0;
        while (path[pos] && path[pos]!='/') {
            if (size==FS_NAME-1) return -36;
            unsigned char c=path[pos++];
            if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.')) return -22;
            name[size++]=(char)c;
        }
        name[size]=0;
        size_t next=pos;while(path[next]=='/')++next;
        if (parent && !path[next]) {
            if (equal(name,".") || equal(name,"..")) return -22;
            memcpy(leaf,name,size+1);return (int)current;
        }
        if (equal(name,".")) continue;
        if (equal(name,"..")) { current=nodes[current].parent;continue; }
        int found=child(current,name);if(found<0)return found;
        current=(unsigned)found;
    }
    if (parent) return -22;
    if (length && path[length-1]=='/' && nodes[current].type!=FS_DIR) return -20;
    return (int)current;
}

static int create(const char *path,unsigned type)
{
    char name[FS_NAME]={0};int parent=resolve(path,true,name);
    if(parent<0)return parent;
    if(child((unsigned)parent,name)>=0)return -17;
    unsigned slot;
    for(slot=1;slot<FS_NODES;++slot)if(!nodes[slot].used)break;
    if(slot==FS_NODES)return -28;
    char *data=0;
    if(type==FS_FILE){data=kmalloc(PAGE_SIZE);if(!data)return -12;memset(data,0,PAGE_SIZE);}
    struct node *node=&nodes[slot];memset(node,0,sizeof(*node));
    node->used=true;node->parent=(unsigned)parent;node->type=type;node->data=data;node->capacity=data?PAGE_SIZE:0;
    memcpy(node->name,name,FS_NAME);
    return (int)slot;
}

int fs_mkdir(const char *path)
{
    uint64_t flags=irq_save();int result=create(path,FS_DIR);irq_restore(flags);
    return result<0?result:0;
}

static struct fs_handle *handle(struct fs_handle *table,int fd)
{
    if(fd<3 || fd>=3+FS_HANDLES || !table[fd-3].node)return 0;
    return &table[fd-3];
}
static void entry(unsigned index,struct fs_entry *result);
static const char *asset_name(const char *path)
{
    static const char prefix[]="/assets/";
    unsigned i=0;for(;prefix[i];++i)if(path[i]!=prefix[i])return 0;
    return path[i]&&path[i]!='/'?path+i:0;
}

int fs_open(struct fs_handle *table,const char *path,unsigned access)
{
    if((access&~31u) || !(access&3) || ((access&FS_TRUNC)&&!(access&FS_WRITE)) ||
       ((access&FS_EXCL)&&!(access&FS_CREATE)))return -22;
    uint64_t flags=irq_save();int result=-24;unsigned slot;
    for(slot=0;slot<FS_HANDLES;++slot)if(!table[slot].node)break;
    if(slot==FS_HANDLES)goto done;
    const char *asset=asset_name(path);
    if(asset){
        if(access&(FS_WRITE|FS_CREATE|FS_TRUNC)){result=-30;goto done;}
        struct asset_file file;result=asset_open(asset,&file);if(result<0)goto done;
        table[slot]=(struct fs_handle){FS_ASSET_NODE,access,0,file};
        result=(int)slot+3;goto done;
    }
    result=resolve(path,false,0);
    if(result>=0&&(access&(FS_CREATE|FS_EXCL))==(FS_CREATE|FS_EXCL)){result=-17;goto done;}
    if(result==-2 && (access&FS_CREATE)) {
        size_t length=0;while(path[length])++length;
        if(length && path[length-1]=='/'){result=-20;goto done;}
        result=create(path,FS_FILE);
    }
    if(result<0)goto done;
    struct node *node=&nodes[result];
    if(node->type!=FS_FILE){result=-21;goto done;}
    if(access&FS_TRUNC){node->size=0;memset(node->data,0,node->capacity);}
    table[slot]=(struct fs_handle){(unsigned)result+1,access,0,{0,0}};
    ++node->references;result=(int)slot+3;
done:
    irq_restore(flags);return result;
}

int fs_close(struct fs_handle *table,int fd)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file){irq_restore(flags);return -9;}
    if(file->node!=FS_ASSET_NODE){
        struct node *node=&nodes[file->node-1];
        if(!--node->references&&node->unlinked){kfree(node->data);memset(node,0,sizeof(*node));}
    }
    memset(file,0,sizeof(*file));irq_restore(flags);return 0;
}
void fs_close_all(struct fs_handle *table)
{
    for(unsigned i=0;i<FS_HANDLES;++i)if(table[i].node)(void)fs_close(table,(int)i+3);
}
int64_t fs_read(struct fs_handle *table,int fd,void *buffer,size_t length)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file || !(file->flags&FS_READ)){irq_restore(flags);return -9;}
    if(file->node==FS_ASSET_NODE){
        size_t count=file->offset>=file->asset.size?0:(size_t)(file->asset.size-file->offset);
        if(count>length)count=length;
        if(count&&!asset_read(&file->asset,file->offset,buffer,count)){irq_restore(flags);return -5;}
        file->offset+=count;irq_restore(flags);return (int64_t)count;
    }
    struct node *node=&nodes[file->node-1];
    size_t count=file->offset>=node->size?0:node->size-file->offset;
    if(count>length)count=length;
    if(count)memcpy(buffer,node->data+file->offset,count);
    file->offset+=count;irq_restore(flags);return (int64_t)count;
}
int64_t fs_write(struct fs_handle *table,int fd,const void *buffer,size_t length)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file || !(file->flags&FS_WRITE)){irq_restore(flags);return -9;}
    if(file->node==FS_ASSET_NODE){irq_restore(flags);return -30;}
    if(length>FS_FILE_MAX-file->offset){irq_restore(flags);return -28;}
    struct node *node=&nodes[file->node-1];
    if(length){
        size_t needed=file->offset+length;
        if(needed>node->capacity){
            size_t capacity=node->capacity;
            while(capacity<needed)capacity*=2;
            char *grown=kmalloc(capacity);
            if(!grown){irq_restore(flags);return -12;}
            memset(grown,0,capacity);
            memcpy(grown,node->data,node->size);
            kfree(node->data);node->data=grown;node->capacity=capacity;
        }
        if(file->offset>node->size)memset(node->data+node->size,0,file->offset-node->size);
        memcpy(node->data+file->offset,buffer,length);file->offset+=length;
        if(file->offset>node->size)node->size=file->offset;
    }
    irq_restore(flags);return (int64_t)length;
}
int64_t fs_seek(struct fs_handle *table,int fd,int64_t offset,int whence)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file){irq_restore(flags);return -9;}
    uint64_t base;
    if(whence==0)base=0;
    else if(whence==1)base=file->offset;
    else if(whence==2)base=file->node==FS_ASSET_NODE?file->asset.size:nodes[file->node-1].size;
    else {irq_restore(flags);return -22;}
    uint64_t position;
    if(offset<0){
        uint64_t amount=0-(uint64_t)offset;
        if(amount>base){irq_restore(flags);return -22;}
        position=base-amount;
    } else {
        uint64_t amount=(uint64_t)offset;
        uint64_t maximum=file->node==FS_ASSET_NODE?file->asset.size:FS_FILE_MAX;
        if(base>maximum||amount>maximum-base){irq_restore(flags);return -22;}
        position=base+amount;
    }
    file->offset=(size_t)position;irq_restore(flags);return (int64_t)position;
}
int fs_fstat(struct fs_handle *table,int fd,struct fs_entry *result)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file){irq_restore(flags);return -9;}
    if(file->node==FS_ASSET_NODE){memset(result,0,sizeof(*result));result->type=FS_FILE;result->size=file->asset.size;result->reserved=1;}
    else entry(file->node-1,result);
    irq_restore(flags);return 0;
}
int fs_truncate(struct fs_handle *table,int fd,size_t length)
{
    if(length>FS_FILE_MAX)return -27;
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file||!(file->flags&FS_WRITE)){irq_restore(flags);return -9;}
    if(file->node==FS_ASSET_NODE){irq_restore(flags);return -30;}
    struct node *node=&nodes[file->node-1];
    if(length>node->capacity){size_t capacity=node->capacity;while(capacity<length)capacity*=2;
        char *grown=kmalloc(capacity);if(!grown){irq_restore(flags);return -12;}
        memset(grown,0,capacity);memcpy(grown,node->data,node->size);kfree(node->data);
        node->data=grown;node->capacity=capacity;
    }
    if(length>node->size)memset(node->data+node->size,0,length-node->size);
    else if(length<node->size)memset(node->data+length,0,node->size-length);
    node->size=length;irq_restore(flags);return 0;
}
int fs_unlink(const char *path)
{
    uint64_t flags=irq_save();int result=resolve(path,false,0);
    if(result<0)goto done;
    if(!result || (nodes[result].type==FS_DIR&&nodes[result].references)){result=-16;goto done;}
    for(unsigned i=1;i<FS_NODES;++i)
        if(nodes[i].used && !nodes[i].unlinked && nodes[i].parent==(unsigned)result){result=-39;goto done;}
    if(nodes[result].references)nodes[result].unlinked=true;
    else {kfree(nodes[result].data);memset(&nodes[result],0,sizeof(nodes[result]));}
    result=0;
done:
    irq_restore(flags);return result;
}
static void entry(unsigned index,struct fs_entry *result)
{
    memset(result,0,sizeof(*result));memcpy(result->name,nodes[index].name,FS_NAME);
    result->type=nodes[index].type;result->size=nodes[index].size;
}
int fs_stat(const char *path,struct fs_entry *result)
{
    if(equal(path,"/assets")||equal(path,"/assets/")){
        memset(result,0,sizeof(*result));result->type=FS_DIR;memcpy(result->name,"assets",7);return 0;
    }
    const char *asset=asset_name(path);if(asset){uint64_t size;int rc=asset_stat(asset,&size);
        if(!rc){memset(result,0,sizeof(*result));result->type=FS_FILE;result->size=size;result->reserved=1;
            size_t i=0;while(i<FS_NAME-1&&asset[i]){result->name[i]=asset[i];++i;}}
        return rc;}
    uint64_t flags=irq_save();int node=resolve(path,false,0);
    if(node>=0)entry((unsigned)node,result);
    irq_restore(flags);return node<0?node:0;
}
int fs_list(const char *path,unsigned position,struct fs_entry *result)
{
    if(equal(path,"/assets")||equal(path,"/assets/")){
        char name[48];uint64_t size;int rc=asset_entry(position,name,&size);if(rc==1){
            memset(result,0,sizeof(*result));result->type=FS_FILE;result->size=size;
            size_t i=0;while(i<FS_NAME-1&&name[i]){result->name[i]=name[i];++i;}}
        return rc;
    }
    uint64_t flags=irq_save();int node=resolve(path,false,0),ret=0;
    if(node<0){ret=node;goto done;}
    if(nodes[node].type!=FS_DIR){ret=-20;goto done;}
    for(unsigned i=1;i<FS_NODES;++i)if(nodes[i].used&&!nodes[i].unlinked&&nodes[i].parent==(unsigned)node){
        if(!position--){entry(i,result);ret=1;break;}
    }
done:
    irq_restore(flags);return ret;
}
unsigned fs_node_count(void)
{
    unsigned count=0;for(unsigned i=0;i<FS_NODES;++i)if(nodes[i].used)++count;return count;
}
void fs_init(void)
{
    nodes[0].used=true;nodes[0].type=FS_DIR;nodes[0].name[0]='/';
    check(!fs_mkdir("/bin")&&!fs_mkdir("/home")&&!fs_mkdir("/etc")&&!fs_mkdir("/dev"),"RAMFS root directories");
    struct fs_handle files[FS_HANDLES]={0};
    int fd=fs_open(files,"/etc/motd",FS_WRITE|FS_CREATE);
    static const char message[]="Welcome to MyOS RAMFS.\n";
    check(fd>=3&&fs_write(files,fd,message,sizeof(message)-1)==sizeof(message)-1,"RAMFS seed file");
    check(!fs_close(files,fd),"RAMFS seed close");
    print("RAMFS mounted: /bin /home /etc /dev (RAM-backed)\n");
}

#include "snapshot-impl.inc"
