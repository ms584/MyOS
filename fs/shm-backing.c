#include "shm-backing.h"
static size_t page_count(size_t size){return (size+PAGE_SIZE-1)/PAGE_SIZE;}
static void copy_range(struct shm_backing *backing,size_t offset,void *buffer,size_t length,bool put)
{
    unsigned char *bytes=buffer;
    while(length){size_t index=offset/PAGE_SIZE,within=offset%PAGE_SIZE;
        size_t chunk=PAGE_SIZE-within;if(chunk>length)chunk=length;
        unsigned char *physical=(void *)backing->pages[index];
        if(put)memcpy(physical+within,bytes,chunk);else memcpy(bytes,physical+within,chunk);
        offset+=chunk;bytes+=chunk;length-=chunk;
    }
}
static void zero_range(struct shm_backing *backing,size_t offset,size_t length)
{
    while(length){size_t within=offset%PAGE_SIZE,chunk=PAGE_SIZE-within;
        if(chunk>length)chunk=length;
        memset((unsigned char *)backing->pages[offset/PAGE_SIZE]+within,0,chunk);
        offset+=chunk;length-=chunk;
    }
}
int shm_resize(struct shm_backing *backing,size_t size)
{
    if(!backing)return -22;if(size>SHM_MAX_BYTES)return -27;
    uint64_t flags=irq_save();size_t old=page_count(backing->size),fresh=page_count(size);
    /* No SIGBUS/demand page invalidation yet. Reject a mapped shrink as one
     * transaction, rather than silently invalidating live user pointers. */
    if(size<backing->size)for(size_t i=0;i<old;++i)
        if(page_references(backing->pages[i])>1){irq_restore(flags);return -16;}
    size_t made=old;
    for(;made<fresh;++made){uintptr_t page=page_alloc();if(!page)break;
        memset((void *)page,0,PAGE_SIZE);backing->pages[made]=page;}
    if(made<fresh){while(made>old){--made;page_free(backing->pages[made]);backing->pages[made]=0;}
        irq_restore(flags);return -12;}
    if(size>backing->size)zero_range(backing,backing->size,size-backing->size);
    else if(size<backing->size&&fresh&&size%PAGE_SIZE)
        zero_range(backing,size,PAGE_SIZE-size%PAGE_SIZE);
    for(size_t i=fresh;i<old;++i){page_free(backing->pages[i]);backing->pages[i]=0;}
    backing->size=size;irq_restore(flags);return 0;
}
void shm_release(struct shm_backing *backing)
{
    if(!backing)return;uint64_t flags=irq_save();
    for(size_t i=0;i<page_count(backing->size);++i)page_free(backing->pages[i]);
    memset(backing,0,sizeof(*backing));irq_restore(flags);
}
int64_t shm_read(struct shm_backing *backing,size_t offset,void *buffer,size_t length)
{
    if(!backing||(!buffer&&length))return -14;
    uint64_t flags=irq_save();size_t count=offset>=backing->size?0:backing->size-offset;
    if(count>length)count=length;if(count)copy_range(backing,offset,buffer,count,false);
    irq_restore(flags);return (int64_t)count;
}
int64_t shm_write(struct shm_backing *backing,size_t offset,const void *buffer,size_t length)
{
    if(!backing||(!buffer&&length))return -14;
    if(offset>SHM_MAX_BYTES||length>SHM_MAX_BYTES-offset)return -28;
    if(!length)return 0;uint64_t flags=irq_save();int result=0;
    if(offset+length>backing->size)result=shm_resize(backing,offset+length);
    if(!result)copy_range(backing,offset,(void *)buffer,length,true);
    irq_restore(flags);return result?result:(int64_t)length;
}
uintptr_t shm_acquire_page(struct shm_backing *backing,size_t offset)
{
    if(!backing||offset%PAGE_SIZE)return 0;uint64_t flags=irq_save();
    uintptr_t page=offset<backing->size?backing->pages[offset/PAGE_SIZE]:0;
    if(page&&!page_retain(page))page=0;irq_restore(flags);return page;
}
