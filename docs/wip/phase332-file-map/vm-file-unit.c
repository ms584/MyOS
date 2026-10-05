#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
static uintptr_t allocated[2048];
static unsigned live,calls,failure_after=~0u;
static uint64_t file_offset=123;
static unsigned read_calls,read_failure_after=~0u;
int fs_fstat(struct fs_handle *table,int fd,struct fs_entry *entry)
{
    (void)table;if(fd!=3)return -9;
    memset(entry,0,sizeof(*entry));entry->type=FS_FILE;entry->reserved=1;entry->size=65537;return 0;
}
int64_t fs_seek(struct fs_handle *table,int fd,int64_t offset,int whence)
{
    (void)table;if(fd!=3)return -9;
    if(whence==1&&offset==0)return (int64_t)file_offset;
    if(whence!=0||offset<0||offset>65537)return -22;
    file_offset=(uint64_t)offset;return offset;
}
int64_t fs_read(struct fs_handle *table,int fd,void *buffer,size_t length)
{
    (void)table;if(fd!=3)return -9;
    if(read_calls++>=read_failure_after)return -5;
    if(length>65537-file_offset)length=(size_t)(65537-file_offset);
    for(size_t i=0;i<length;++i)((unsigned char *)buffer)[i]=(unsigned char)((file_offset+i)*37+19);
    file_offset+=length;return (int64_t)length;
}
uintptr_t page_alloc(void)
{
    if(calls++>=failure_after||live==2048)return 0;
    void *p=aligned_alloc(4096,4096);assert(p);memset(p,0,4096);
    allocated[live++]=(uintptr_t)p;return (uintptr_t)p;
}
void page_free(uintptr_t page)
{
    for(unsigned i=0;i<live;++i)if(allocated[i]==page){free((void *)page);allocated[i]=allocated[--live];return;}
    assert(!"double/unowned page free");
}
size_t pages_free(void){return 2048-live;}
void *kmalloc(size_t size){return malloc(size);}
void kfree(void *p){free(p);}
void check(bool ok,const char *text){(void)text;assert(ok);}
_Noreturn void panic(const char *text){(void)text;abort();}
extern long write(int,const void *,size_t);
static uint64_t *leaf(struct process *p,uint64_t address)
{
    uint64_t *table=(void *)p->root;
    for(int shift=39;shift>=21;shift-=9){uint64_t e=table[(address>>shift)&511];assert(e&1);table=(void *)(uintptr_t)(e&0x000ffffffffff000ull);}
    return &table[(address>>12)&511];
}
static unsigned char *data(struct process *p,uint64_t address)
{return (void *)(uintptr_t)(*leaf(p,address)&0x000ffffffffff000ull);}
int main(void)
{
    struct process p={0};p.root=page_alloc();
    uint64_t low=PROCESS_VM_BASE,high=PROCESS_VM_HIGH_BASE;
    assert(process_vm_map(&p,low,4*4096,3)==(int64_t)low);
    data(&p,low)[0]=91;
    assert(process_vm_map(&p,high,0x800000000ull,0)==(int64_t)high);
    unsigned baseline=live,ledger=p.vm_page_count;
    /* Inject failure after zero through six successful page/table allocations.
     * The exact source implementation must roll back all staged allocations. */
    for(unsigned n=0;n<7;++n){
        failure_after=calls+n;
        assert(process_vm_protect(&p,high,16*4096,3)==-12);
        failure_after=~0u;
        assert(live==baseline&&p.vm_page_count==ledger);
        assert(process_vm_mapped(&p,high,0x800000000ull));
        assert(!process_vm_resident(&p,high)&&!process_user_access(&p,high,1,1));
        assert(data(&p,low)[0]==91&&process_user_access(&p,low,1,3));
    }
    assert(!process_vm_protect(&p,high,16*4096,3));
    for(unsigned i=0;i<16;++i){assert(process_vm_resident(&p,high+i*4096));assert(!data(&p,high+i*4096)[0]);}
    data(&p,high)[0]=73;
    assert(!process_vm_protect(&p,high,4096,0));
    assert(!process_user_access(&p,high,1,1)&&process_vm_resident(&p,high));
    assert(!process_vm_protect(&p,high,4096,3)&&data(&p,high)[0]==73);
    assert(!process_vm_unmap(&p,high,0x800000000ull)&&live==baseline);
    assert(!process_vm_unmap(&p,low,4*4096)&&live==1&&!p.vm_page_count&&!p.vm_areas);
    /* Failed fresh mappings must leave no reservation or empty page tables. */
    for(unsigned n=0;n<8;++n){failure_after=calls+n;
        assert(process_vm_map(&p,high,16*4096,3)==-12);failure_after=~0u;
        assert(live==1&&!p.vm_page_count&&!p.vm_areas);}
    assert(process_vm_map(&p,high,UINT64_MAX,0)==-22);
    assert(process_vm_map(&p,PROCESS_VM_END,4096,0)==-22);
    assert(process_vm_map(&p,high,4096,6)==-22);
    struct vm_file_request request={high,16*4096,0,3,1,VM_FILE_SHARED};
    for(unsigned n=0;n<8;++n){failure_after=calls+n;
        assert(process_vm_file(&p,&request)==-12);failure_after=~0u;
        assert(live==1&&!p.vm_page_count&&!p.vm_areas&&file_offset==123);
    }
    /* Fail actual backing reads after each partial prefix: offsets/reservation
     * and every physical/page-table allocation must return exactly. */
    for(unsigned n=0;n<16;++n){read_failure_after=read_calls+n;
        assert(process_vm_file(&p,&request)==-5);read_failure_after=~0u;
        assert(live==1&&!p.vm_page_count&&!p.vm_areas&&file_offset==123);
    }
    assert(process_vm_file(&p,&request)==(int64_t)high&&file_offset==123);
    assert(data(&p,high)[0]==19&&!process_user_access(&p,high,1,3));
    assert(process_vm_protect(&p,high,4096,3)==-13);
    assert(!process_vm_protect(&p,high,4096,0));
    assert(process_vm_protect(&p,high,4096,3)==-13);
    assert(!process_vm_protect(&p,high,4096,1)&&data(&p,high)[0]==19);
    assert(process_vm_advise(&p,high,16*4096,4)==-95&&data(&p,high)[0]==19);
    assert(!process_vm_unmap(&p,high,16*4096)&&live==1&&!p.vm_areas);
    free(p.vm_pages);page_free(p.root);assert(!live);
    const char message[]="PASS real VM/file-map allocator and partial-read failure rollback, offsets, immutable rights and exact cleanup\n";
    write(1,message,sizeof(message)-1);
}
