/* Included by ramfs.c; snapshots never expose the live node table. */
static const struct fs_disk_meta *snapshot_meta(const void *slots,size_t stride,unsigned i)
{return (const struct fs_disk_meta *)((const uint8_t *)slots+(size_t)i*stride);}
static struct fs_disk_meta *snapshot_mut(void *slots,size_t stride,unsigned i)
{return (struct fs_disk_meta *)((uint8_t *)slots+(size_t)i*stride);}
static bool snapshot_name(const char name[FS_NAME])
{
    size_t n=0;
    while(n<FS_NAME&&name[n]){
        unsigned char c=(unsigned char)name[n++];
        if(!fs_name_byte(c))return false;
    }
    return n>0&&n<FS_NAME&&!(n==1&&name[0]=='.')&&
           !(n==2&&name[0]=='.'&&name[1]=='.');
}
static bool snapshot_shape(size_t stride,size_t max_file)
{return (max_file==FS_V1_FILE_MAX||max_file==FS_FILE_MAX)&&stride==STORAGE_SECTOR_SIZE+max_file;}
bool fs_snapshot_export(void *slots,size_t stride,size_t max_file)
{
    if(!slots||!snapshot_shape(stride,max_file))return false;
    uint64_t flags=irq_save();
    for(unsigned i=0;i<FS_NODES;++i)
        if(nodes[i].used&&!nodes[i].unlinked&&!shared_namespace(i)&&nodes[i].type==FS_FILE&&nodes[i].size>max_file){irq_restore(flags);return false;}
    memset(slots,0,stride*FS_NODES);
    for(unsigned i=0;i<FS_NODES;++i){
        if(!nodes[i].used||nodes[i].unlinked||shared_namespace(i))continue;
        struct fs_disk_meta *slot=snapshot_mut(slots,stride,i);
        slot->used=1;slot->parent=nodes[i].parent;slot->type=nodes[i].type;
        slot->size=(uint32_t)nodes[i].size;memcpy(slot->name,nodes[i].name,FS_NAME);
        if(nodes[i].type==FS_FILE&&nodes[i].size)
            memcpy((uint8_t *)slot+STORAGE_SECTOR_SIZE,nodes[i].data,nodes[i].size);
    }
    irq_restore(flags);return true;
}
static bool valid_snapshot(const void *slots,size_t stride,size_t max_file)
{
    const struct fs_disk_meta *root=snapshot_meta(slots,stride,0);
    if(root->used!=1||root->type!=FS_DIR||root->parent||root->size||root->name[0]!='/'||root->name[1])return false;
    for(unsigned i=1;i<FS_NODES;++i){
        const struct fs_disk_meta *slot=snapshot_meta(slots,stride,i);
        if(slot->used>1)return false;
        if(!slot->used)continue;
        const struct fs_disk_meta *parent=snapshot_meta(slots,stride,slot->parent<FS_NODES?slot->parent:0);
        if(slot->parent>=FS_NODES||!parent->used||parent->type!=FS_DIR||
           (slot->type!=FS_FILE&&slot->type!=FS_DIR)||slot->size>max_file||
           (slot->type==FS_DIR&&slot->size)||!snapshot_name(slot->name))return false;
        unsigned p=slot->parent;
        for(unsigned depth=0;p;++depth){
            if(depth>=FS_NODES||p==i)return false;
            const struct fs_disk_meta *ancestor=snapshot_meta(slots,stride,p);
            p=ancestor->parent;
            if(p>=FS_NODES||!snapshot_meta(slots,stride,p)->used)return false;
        }
        for(unsigned j=1;j<i;++j){
            const struct fs_disk_meta *other=snapshot_meta(slots,stride,j);
            if(other->used&&other->parent==slot->parent&&equal(other->name,slot->name))return false;
        }
    }
    return true;
}
bool fs_snapshot_valid(const void *slots,size_t stride,size_t max_file)
{return slots&&snapshot_shape(stride,max_file)&&valid_snapshot(slots,stride,max_file);}
bool fs_snapshot_import(const void *slots,size_t stride,size_t max_file)
{
    if(!fs_snapshot_valid(slots,stride,max_file))return false;
    char *fresh[FS_NODES]={0};size_t capacities[FS_NODES]={0};
    for(unsigned i=1;i<FS_NODES;++i){
        const struct fs_disk_meta *slot=snapshot_meta(slots,stride,i);
        if(!slot->used||slot->type!=FS_FILE)continue;
        size_t capacity=PAGE_SIZE;
        while(capacity<slot->size)capacity*=2;
        fresh[i]=kmalloc(capacity);if(!fresh[i])goto failed;
        capacities[i]=capacity;memset(fresh[i],0,capacity);
        if(slot->size)memcpy(fresh[i],(const uint8_t *)slot+STORAGE_SECTOR_SIZE,slot->size);
    }
    uint64_t flags=irq_save();
    for(unsigned i=0;i<FS_LIVE_NODES;++i)if(nodes[i].references){irq_restore(flags);goto failed;}
    for(unsigned i=0;i<FS_LIVE_NODES;++i)release_node_data(&nodes[i]);
    memset(nodes,0,sizeof(nodes));
    for(unsigned i=0;i<FS_NODES;++i){
        const struct fs_disk_meta *slot=snapshot_meta(slots,stride,i);
        if(!slot->used)continue;
        nodes[i].used=true;nodes[i].parent=slot->parent;nodes[i].type=slot->type;
        nodes[i].size=slot->size;nodes[i].data=fresh[i];nodes[i].capacity=capacities[i];fresh[i]=0;
        memcpy(nodes[i].name,slot->name,FS_NAME);
    }
    irq_restore(flags);return true;
failed:
    for(unsigned i=0;i<FS_NODES;++i)kfree(fresh[i]);
    return false;
}
