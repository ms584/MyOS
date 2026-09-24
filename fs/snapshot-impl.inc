/* Included by ramfs.c so the disk format never exposes the live node table. */
static bool snapshot_name(const char name[FS_NAME])
{
    size_t n=0;
    while(n<FS_NAME && name[n]){
        unsigned char c=(unsigned char)name[n++];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
    }
    return n>0 && n<FS_NAME && !(n==1&&name[0]=='.') &&
           !(n==2&&name[0]=='.'&&name[1]=='.');
}
bool fs_snapshot_export(struct fs_disk_slot *slots)
{
    if(!slots)return false;
    uint64_t flags=irq_save();
    memset(slots,0,sizeof(*slots)*FS_NODES);
    for(unsigned i=0;i<FS_NODES;++i){
        if(!nodes[i].used)continue;
        struct fs_disk_slot *slot=&slots[i];
        slot->used=1;slot->parent=nodes[i].parent;slot->type=nodes[i].type;
        slot->size=(uint32_t)nodes[i].size;
        memcpy(slot->name,nodes[i].name,FS_NAME);
        if(nodes[i].type==FS_FILE&&nodes[i].size)
            memcpy(slot->data,nodes[i].data,nodes[i].size);
    }
    irq_restore(flags);return true;
}
static bool valid_snapshot(const struct fs_disk_slot *slots)
{
    if(slots[0].used!=1 || slots[0].type!=FS_DIR || slots[0].parent ||
       slots[0].size || slots[0].name[0]!='/' || slots[0].name[1])return false;
    for(unsigned i=1;i<FS_NODES;++i){
        const struct fs_disk_slot *slot=&slots[i];
        if(slot->used>1)return false;
        if(!slot->used)continue;
        if(slot->parent>=FS_NODES || !slots[slot->parent].used ||
           slots[slot->parent].type!=FS_DIR ||
           (slot->type!=FS_FILE&&slot->type!=FS_DIR) ||
           slot->size>FS_FILE_MAX ||
           (slot->type==FS_DIR&&slot->size) || !snapshot_name(slot->name))return false;
        unsigned parent=slot->parent;
        for(unsigned depth=0;parent; ++depth){
            if(depth>=FS_NODES || parent==i)return false;
            parent=slots[parent].parent;
            if(parent>=FS_NODES || !slots[parent].used)return false;
        }
        for(unsigned j=1;j<i;++j){
            if(slots[j].used && slots[j].parent==slot->parent &&
               equal(slots[j].name,slot->name))return false;
        }
    }
    return true;
}
bool fs_snapshot_valid(const struct fs_disk_slot *slots)
{
    return slots && valid_snapshot(slots);
}
bool fs_snapshot_import(const struct fs_disk_slot *slots)
{
    if(!slots || !valid_snapshot(slots))return false;
    char *fresh[FS_NODES]={0};
    for(unsigned i=1;i<FS_NODES;++i)if(slots[i].used&&slots[i].type==FS_FILE){
        fresh[i]=kmalloc(FS_FILE_MAX);
        if(!fresh[i])goto failed;
        memset(fresh[i],0,FS_FILE_MAX);
        if(slots[i].size)memcpy(fresh[i],slots[i].data,slots[i].size);
    }
    uint64_t flags=irq_save();
    for(unsigned i=0;i<FS_NODES;++i)if(nodes[i].references){irq_restore(flags);goto failed;}
    for(unsigned i=0;i<FS_NODES;++i)kfree(nodes[i].data);
    memset(nodes,0,sizeof(nodes));
    for(unsigned i=0;i<FS_NODES;++i)if(slots[i].used){
        nodes[i].used=true;nodes[i].parent=slots[i].parent;nodes[i].type=slots[i].type;
        nodes[i].size=slots[i].size;nodes[i].data=fresh[i];fresh[i]=0;
        memcpy(nodes[i].name,slots[i].name,FS_NAME);
    }
    irq_restore(flags);return true;
failed:
    for(unsigned i=0;i<FS_NODES;++i)kfree(fresh[i]);
    return false;
}
