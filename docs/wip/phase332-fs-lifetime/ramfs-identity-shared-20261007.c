#include "fs-snapshot.h"
#include "shm-backing.h"
#define FS_ASSET_NODE UINT32_MAX
struct node {
    bool used, unlinked;
    unsigned parent, type, references;
    size_t size, capacity;
    char name[FS_NAME];
    char *data;
    struct shm_backing *shared;
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
static bool shared_namespace(unsigned index)
{
    for(unsigned depth=0;index&&depth<FS_NODES;++depth){
        unsigned parent=nodes[index].parent;
        if(equal(nodes[index].name,"shm")&&parent&&nodes[parent].parent==0&&
           equal(nodes[parent].name,"dev"))return true;
        index=parent;
    }
    return false;
}
static void release_node_data(struct node *node)
{
    kfree(node->data);node->data=0;
    if(node->shared){shm_release(node->shared);kfree(node->shared);node->shared=0;}
}

/* Relative paths use a pinned directory inode, not a reconstructed name. */
static int resolve_from(const char *path,unsigned start,bool parent,char *leaf)
{
    if (!path || !path[0]) return -22;
    size_t length=0;
    while (length<FS_PATH && path[length]) ++length;
    if (length==FS_PATH) return -36;
    unsigned current=path[0]=='/'?0:start;
    if(current>=FS_NODES||!nodes[current].used||nodes[current].unlinked)return -2;
    size_t pos=path[0]=='/';
    while (pos<length) {
        while (path[pos]=='/') ++pos;
        if (!path[pos]) break;
        if (nodes[current].type!=FS_DIR) return -20;
        char name[FS_NAME]; size_t size=0;
        while (path[pos] && path[pos]!='/') {
            if (size==FS_NAME-1) return -36;
            unsigned char c=path[pos++];
            if (!fs_name_byte(c)) return -22;
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
static int resolve(const char *path,bool parent,char *leaf)
{if(!path||path[0]!='/')return -22;return resolve_from(path,0,parent,leaf);}

static int create_from(const char *path,unsigned start,unsigned type)
{
    char name[FS_NAME]={0};int parent=resolve_from(path,start,true,name);
    if(parent<0)return parent;
    if(child((unsigned)parent,name)>=0)return -17;
    unsigned slot;
    for(slot=1;slot<FS_NODES;++slot)if(!nodes[slot].used)break;
    if(slot==FS_NODES)return -28;
    char *data=0;struct shm_backing *shared=0;
    if(type==FS_FILE){
        if(shared_namespace((unsigned)parent)){
            shared=kmalloc(sizeof(*shared));if(!shared)return -12;memset(shared,0,sizeof(*shared));
        }else{data=kmalloc(PAGE_SIZE);if(!data)return -12;memset(data,0,PAGE_SIZE);}
    }
    struct node *node=&nodes[slot];memset(node,0,sizeof(*node));
    node->used=true;node->parent=(unsigned)parent;node->type=type;node->data=data;node->capacity=data?PAGE_SIZE:0;
    node->shared=shared;
    memcpy(node->name,name,FS_NAME);
    return (int)slot;
}
static int create(const char *path,unsigned type)
{if(!path||path[0]!='/')return -22;return create_from(path,0,type);}

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

/* Same IRQ exclusion covers choosing the directory and resolving the name. */
static int directory_base(struct fs_handle *table,int directory,const char *path)
{
    if(!path||!path[0])return -22;
    if(path[0]=='/'||directory==FS_AT_FDCWD)return 0;
    if(!table)return -9;
    struct fs_handle *file=handle(table,directory);if(!file)return -9;
    if(file->node==FS_ASSET_NODE)return -20;
    unsigned index=file->node-1;
    if(nodes[index].type!=FS_DIR)return -20;
    if(nodes[index].unlinked)return -2;
    return (int)index;
}
int fs_open_at(struct fs_handle *table,int directory,const char *path,unsigned access)
{
    if((access&~127u) || !(access&3) || ((access&FS_TRUNC)&&!(access&FS_WRITE)) ||
       ((access&FS_EXCL)&&!(access&FS_CREATE))||
       ((access&FS_AUTO_DIRECTORY)&&(access&(FS_WRITE|FS_CREATE|FS_TRUNC|FS_EXCL)))||
       ((access&FS_DIRECTORY)&&(access&(FS_WRITE|FS_CREATE|FS_TRUNC|FS_EXCL))))return -22;
    uint64_t flags=irq_save();int result=-24;unsigned slot;
    int base=directory_base(table,directory,path);if(base<0){result=base;goto done;}
    for(slot=0;slot<FS_HANDLES;++slot)if(!table[slot].node)break;
    if(slot==FS_HANDLES)goto done;
    const char *asset=asset_name(path);
    if(asset){
        if(access&FS_DIRECTORY){result=-95;goto done;}
        if(access&(FS_WRITE|FS_CREATE|FS_TRUNC)){result=-30;goto done;}
        struct asset_file file;result=asset_open(asset,&file);
        if(result<0){if(result==-2&&!asset_directory(asset))result=-21;goto done;}
        table[slot]=(struct fs_handle){FS_ASSET_NODE,access,0,file};
        result=(int)slot+3;goto done;
    }
    result=resolve_from(path,(unsigned)base,false,0);
    if(result>=0&&(access&(FS_CREATE|FS_EXCL))==(FS_CREATE|FS_EXCL)){result=-17;goto done;}
    if(result==-2 && (access&FS_CREATE)) {
        size_t length=0;while(path[length])++length;
        if(length && path[length-1]=='/'){result=-20;goto done;}
        result=create_from(path,(unsigned)base,FS_FILE);
    }
    if(result<0)goto done;
    struct node *node=&nodes[result];
    if(access&FS_DIRECTORY){if(node->type!=FS_DIR){result=-20;goto done;}}
    else if(node->type!=FS_FILE&&!(access&FS_AUTO_DIRECTORY)){result=-21;goto done;}
    if(access&FS_TRUNC){
        if(node->shared){int rc=shm_resize(node->shared,0);if(rc){result=rc;goto done;}}
        else memset(node->data,0,node->capacity);
        node->size=0;
    }
    table[slot]=(struct fs_handle){(unsigned)result+1,access,0,{0,0}};
    ++node->references;result=(int)slot+3;
done:
    irq_restore(flags);return result;
}
int fs_open(struct fs_handle *table,const char *path,unsigned access)
{if(!path||path[0]!='/')return -22;return fs_open_at(table,FS_AT_FDCWD,path,access);}

int fs_close(struct fs_handle *table,int fd)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file){irq_restore(flags);return -9;}
    if(file->node!=FS_ASSET_NODE){
        record_release(file->node,(uintptr_t)table);
        struct node *node=&nodes[file->node-1];
        if(!--node->references&&node->unlinked){release_node_data(node);memset(node,0,sizeof(*node));}
    }
    memset(file,0,sizeof(*file));irq_restore(flags);return 0;
}
void fs_close_all(struct fs_handle *table)
{
    for(unsigned i=0;i<FS_HANDLES;++i)if(table[i].node)(void)fs_close(table,(int)i+3);
}
int fs_record_release(struct fs_handle *table,int fd)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    int rc=file?0:-9;
    if(file&&file->node!=FS_ASSET_NODE)record_release(file->node,(uintptr_t)table);
    irq_restore(flags);return rc;
}
int fs_record_lock(struct fs_handle *table,int fd,int32_t pid,unsigned command,struct record_lock_abi *request)
{
    if(!request)return -14;
    uint64_t flags=irq_save();int rc=-9;struct fs_handle *file=handle(table,fd);
    if(!file)goto done;
    if(file->node==FS_ASSET_NODE){rc=-95;goto done;}
    struct node *node=&nodes[file->node-1];
    if(node->type!=FS_FILE){rc=-21;goto done;}
    if(request->type<RECORD_READ||request->type>RECORD_UNLOCK||
       request->whence<0||request->whence>2){rc=-22;goto done;}
    if(command==RECORD_SET&&
       ((request->type==RECORD_READ&&!(file->flags&FS_READ))||
        (request->type==RECORD_WRITE&&!(file->flags&FS_WRITE))))goto done;
    uint64_t base=request->whence==0?0:request->whence==1?file->offset:node->size;
    uint64_t start,end;
    if(request->start<0){uint64_t amount=0-(uint64_t)request->start;
        if(amount>base){rc=-22;goto done;}start=base-amount;
    }else{
        if(base>INT64_MAX||(uint64_t)request->start>(uint64_t)INT64_MAX-base){rc=-75;goto done;}
        start=base+(uint64_t)request->start;
    }
    if(request->length<0){uint64_t amount=0-(uint64_t)request->length;
        if(amount>start){rc=-22;goto done;}end=start;start-=amount;
    }else if(!request->length)end=UINT64_C(1)<<63;
    else{
        if((uint64_t)request->length>(UINT64_C(1)<<63)-start){rc=-75;goto done;}
        end=start+(uint64_t)request->length;
    }
    rc=record_apply(file->node,(uintptr_t)table,pid,command,request->type,start,end,request);
done:
    irq_restore(flags);return rc;
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
    if(node->type==FS_DIR){irq_restore(flags);return -21;}
    if(node->shared){int64_t count=shm_read(node->shared,file->offset,buffer,length);
        if(count>0)file->offset+=(size_t)count;irq_restore(flags);return count;}
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
    struct node *node=&nodes[file->node-1];
    if(node->type==FS_DIR){irq_restore(flags);return -21;}
    if(node->shared){int64_t count=shm_write(node->shared,file->offset,buffer,length);
        if(count>0)file->offset+=(size_t)count;node->size=node->shared->size;
        irq_restore(flags);return count;}
    if(length>FS_FILE_MAX-file->offset){irq_restore(flags);return -28;}
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
    if(file->node!=FS_ASSET_NODE&&nodes[file->node-1].type==FS_DIR){irq_restore(flags);return -29;}
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
        uint64_t maximum=file->node==FS_ASSET_NODE?file->asset.size:
            nodes[file->node-1].shared?SHM_MAX_BYTES:FS_FILE_MAX;
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
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);
    if(!file||!(file->flags&FS_WRITE)){irq_restore(flags);return -9;}
    if(file->node==FS_ASSET_NODE){irq_restore(flags);return -30;}
    struct node *node=&nodes[file->node-1];
    if(node->shared){int rc=shm_resize(node->shared,length);if(!rc)node->size=length;
        irq_restore(flags);return rc;}
    if(length>FS_FILE_MAX){irq_restore(flags);return -27;}
    if(length>node->capacity){size_t capacity=node->capacity;while(capacity<length)capacity*=2;
        char *grown=kmalloc(capacity);if(!grown){irq_restore(flags);return -12;}
        memset(grown,0,capacity);memcpy(grown,node->data,node->size);kfree(node->data);
        node->data=grown;node->capacity=capacity;
    }
    if(length>node->size)memset(node->data+node->size,0,length-node->size);
    else if(length<node->size)memset(node->data+length,0,node->size-length);
    node->size=length;irq_restore(flags);return 0;
}
static int unlink_from(struct fs_handle *table,int directory,const char *path,unsigned mode)
{
    uint64_t flags=irq_save();int result;
    if(!path||!path[0]){result=-22;goto done;}
    if(asset_name(path)){result=-30;goto done;}
    result=directory_base(table,directory,path);if(result<0)goto done;
    result=resolve_from(path,(unsigned)result,false,0);
    if(result<0)goto done;
    if(mode==0&&nodes[result].type==FS_DIR){result=-21;goto done;}
    if(mode==1&&nodes[result].type!=FS_DIR){result=-20;goto done;}
    if(!result || (nodes[result].type==FS_DIR&&nodes[result].references)){result=-16;goto done;}
    for(unsigned i=1;i<FS_NODES;++i)
        if(nodes[i].used && !nodes[i].unlinked && nodes[i].parent==(unsigned)result){result=-39;goto done;}
    if(nodes[result].references)nodes[result].unlinked=true;
    else {release_node_data(&nodes[result]);memset(&nodes[result],0,sizeof(nodes[result]));}
    result=0;
done:
    irq_restore(flags);return result;
}
int fs_unlink_at(struct fs_handle *table,int directory,const char *path,unsigned remove_directory)
{
    if(remove_directory>1)return -22;
    return unlink_from(table,directory,path,remove_directory);
}
int fs_unlink(const char *path)
{return unlink_from(0,FS_AT_FDCWD,path,2);}
static void entry(unsigned index,struct fs_entry *result)
{
    memset(result,0,sizeof(*result));memcpy(result->name,nodes[index].name,FS_NAME);
    result->type=nodes[index].type;result->size=nodes[index].size;
    if(nodes[index].shared)result->reserved=2;
}
int fs_shared_rights(struct fs_handle *table,int fd,unsigned *rights)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);int rc=-9;
    if(file){rc=-95;if(file->node!=FS_ASSET_NODE&&nodes[file->node-1].shared){
        *rights=file->flags&3u;rc=0;}}
    irq_restore(flags);return rc;
}
uintptr_t fs_shared_page(struct fs_handle *table,int fd,size_t offset)
{
    uint64_t flags=irq_save();struct fs_handle *file=handle(table,fd);uintptr_t page=0;
    if(file&&file->node!=FS_ASSET_NODE&&nodes[file->node-1].shared)
        page=shm_acquire_page(nodes[file->node-1].shared,offset);
    irq_restore(flags);return page;
}
int fs_stat(const char *path,struct fs_entry *result)
{
    if(equal(path,"/assets")||equal(path,"/assets/")){
        memset(result,0,sizeof(*result));result->type=FS_DIR;memcpy(result->name,"assets",7);return 0;
    }
    const char *asset=asset_name(path);if(asset){uint64_t size=0;int rc=asset_stat(asset,&size);unsigned type=FS_FILE;
        if(rc==-2&&!asset_directory(asset)){rc=0;type=FS_DIR;}
        if(!rc){memset(result,0,sizeof(*result));result->type=type;result->size=size;result->reserved=1;
            const char *leaf=asset;for(const char *p=asset;*p;++p)if(*p=='/')leaf=p+1;
            size_t i=0;while(i<FS_NAME-1&&leaf[i]){result->name[i]=leaf[i];++i;}}
        return rc;}
    uint64_t flags=irq_save();int node=resolve(path,false,0);
    if(node>=0)entry((unsigned)node,result);
    irq_restore(flags);return node<0?node:0;
}
int fs_stat_at(struct fs_handle *table,int directory,const char *path,struct fs_entry *result)
{
    if(path&&path[0]=='/')return fs_stat(path,result);
    uint64_t flags=irq_save();int node=directory_base(table,directory,path);
    if(node>=0)node=resolve_from(path,(unsigned)node,false,0);
    if(node>=0)entry((unsigned)node,result);
    irq_restore(flags);return node<0?node:0;
}
static bool asset_path(const char *path)
{return path&&(equal(path,"/assets")||equal(path,"/assets/")||asset_name(path));}
static bool contains_shared_namespace(unsigned source)
{
    for(unsigned i=1;i<FS_NODES;++i){
        if(!nodes[i].used||nodes[i].unlinked||(!nodes[i].shared&&!shared_namespace(i)))continue;
        unsigned index=i;
        for(unsigned depth=0;index&&depth<FS_NODES;++depth){
            if(index==source)return true;index=nodes[index].parent;
        }
    }
    return false;
}
int fs_rename_at(struct fs_handle *table,int old_directory,const char *old_path,
                 int new_directory,const char *new_path)
{
    if(asset_path(old_path)||asset_path(new_path))return -30;
    uint64_t flags=irq_save();int result=directory_base(table,old_directory,old_path);
    if(result<0)goto done;
    int source=resolve_from(old_path,(unsigned)result,false,0);if(source<0){result=source;goto done;}
    if(!source){result=-16;goto done;}
    result=directory_base(table,new_directory,new_path);if(result<0)goto done;
    char name[FS_NAME]={0};int parent=resolve_from(new_path,(unsigned)result,true,name);
    if(parent<0){result=parent;goto done;}
    size_t length=0;while(new_path[length])++length;
    if(new_path[length-1]=='/'&&nodes[source].type!=FS_DIR){result=-20;goto done;}
    int destination=child((unsigned)parent,name);
    if(destination==source){result=0;goto done;}
    /* Shared memory is an ephemeral volume. Crossing it requires real copy /
     * persistence conversion, not moving a page-backed inode into snapshots.
     * Also forbid relocating its ancestor: the path-based backing designation
     * would otherwise disappear while live mappings still refer to frames. */
    bool shared_source=shared_namespace((unsigned)source);
    bool shared_parent=shared_namespace((unsigned)parent);
    if(shared_source!=shared_parent||(!shared_source&&contains_shared_namespace((unsigned)source))){
        result=-18;goto done;
    }
    if(nodes[source].type==FS_DIR){
        unsigned ancestor=(unsigned)parent;
        for(unsigned depth=0;;++depth){
            if(ancestor==(unsigned)source||depth>=FS_NODES){result=-22;goto done;}
            if(!ancestor)break;ancestor=nodes[ancestor].parent;
        }
    }
    if(destination>=0){
        if(nodes[source].type==FS_DIR&&nodes[destination].type!=FS_DIR){result=-20;goto done;}
        if(nodes[source].type!=FS_DIR&&nodes[destination].type==FS_DIR){result=-21;goto done;}
        if(nodes[destination].type==FS_DIR)for(unsigned i=1;i<FS_NODES;++i)
            if(nodes[i].used&&!nodes[i].unlinked&&nodes[i].parent==(unsigned)destination){result=-39;goto done;}
    }
    /* No allocation or failing operation after validation. Replaced open
     * inodes retain their own heap/page backing until final close. */
    if(destination>=0){
        if(nodes[destination].references)nodes[destination].unlinked=true;
        else {release_node_data(&nodes[destination]);memset(&nodes[destination],0,sizeof(nodes[destination]));}
    }
    nodes[source].parent=(unsigned)parent;memcpy(nodes[source].name,name,FS_NAME);result=0;
done:
    irq_restore(flags);return result;
}
int fs_rename(const char *old_path,const char *new_path)
{if(!old_path||!new_path||old_path[0]!='/'||new_path[0]!='/')return -22;
 return fs_rename_at(0,FS_AT_FDCWD,old_path,FS_AT_FDCWD,new_path);}
int fs_list(const char *path,unsigned position,struct fs_entry *result)
{
    const char *asset=asset_name(path);
    if(equal(path,"/assets")||equal(path,"/assets/")||asset){
        char name[48];uint64_t size;unsigned type;
        int rc=asset_child(asset?asset:"",position,name,&size,&type);if(rc==1){
            memset(result,0,sizeof(*result));result->type=type;result->size=size;result->reserved=1;
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
int fs_list_at(struct fs_handle *table,int directory,const char *path,unsigned position,struct fs_entry *result)
{
    if(path&&path[0]=='/')return fs_list(path,position,result);
    uint64_t flags=irq_save();int node=directory_base(table,directory,path),ret=0;
    if(node>=0)node=resolve_from(path,(unsigned)node,false,0);
    if(node<0){ret=node;goto done;}
    if(nodes[node].type!=FS_DIR){ret=-20;goto done;}
    for(unsigned i=1;i<FS_NODES;++i)if(nodes[i].used&&!nodes[i].unlinked&&nodes[i].parent==(unsigned)node)
        if(!position--){entry(i,result);ret=1;break;}
done:
    irq_restore(flags);return ret;
}
unsigned fs_node_count(void)
{
    unsigned count=0;for(unsigned i=0;i<FS_NODES;++i)if(nodes[i].used)++count;return count;
}
int fs_volume_stat(const char *path,struct fs_volume_info *result)
{
    if(!path||!result)return -14;
    /* Assets are a separate immutable volume. Its aggregate allocation
     * metadata is not exposed by this RAMFS query; do not invent it. */
    if(equal(path,"/assets")||equal(path,"/assets/")||asset_name(path))return -95;
    uint64_t flags=irq_save();int node=resolve(path,false,0);
    if(node>=0){
        unsigned used=fs_node_count();memset(result,0,sizeof(*result));
        result->block_size=PAGE_SIZE;result->fragment_size=PAGE_SIZE;
        /* RAMFS has no filesystem-wide block pool/quota. Zero block fields
         * express that absence, not host free disk or guaranteed free RAM.
         * Per-file65536 bound and dynamic kmalloc failures still apply. */
        result->files=FS_NODES;result->files_free=FS_NODES-used;
        result->files_available=result->files_free;result->filesystem_id=1;
        result->flags=2; /* ST_NOSUID: RAMFS has no setuid executable modes. */
        result->name_max=FS_NAME-1;
    }
    irq_restore(flags);return node<0?node:0;
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
