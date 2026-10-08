#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
static uintptr_t allocated[2048];
static unsigned live,calls,failure_after=~0u;
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
    /* Cross several 2 MiB leaf-table boundaries; partial unmap must retain
     * both edge tables/data, while empty middle tables are freed exactly. */
    uint64_t cross=high+0x1ff000;size_t span=1026*4096;
    assert(process_vm_map(&p,cross,span,3)==(int64_t)cross);
    data(&p,cross)[0]=17;data(&p,cross+span-4096)[0]=29;
    assert(!process_vm_unmap(&p,cross+4096,span-8192));
    assert(data(&p,cross)[0]==17&&data(&p,cross+span-4096)[0]==29);
    assert(!process_vm_resident(&p,cross+4096));
    assert(!process_vm_resident(&p,cross+span-8192));
    assert(!process_vm_unmap(&p,cross,span));
    assert(live==1&&!p.vm_page_count&&!p.vm_areas);
    /* Staged rollback spanning table boundaries must reclaim all new data and
     * table pages, including a partially filled final table. */
    unsigned injected[]={510,511,512,513,514,515,1020};
    for(unsigned i=0;i<sizeof(injected)/sizeof(injected[0]);++i){
        failure_after=calls+injected[i];
        assert(process_vm_map(&p,cross,span,3)==-12);failure_after=~0u;
        assert(live==1&&!p.vm_page_count&&!p.vm_areas);
    }
    free(p.vm_pages);page_free(p.root);assert(!live);
    const char message[]="PASS actual VM commit rollback/rights plus cross-2MiB partial unmap and staged failure cleanup\n";
    write(1,message,sizeof(message)-1);
}
