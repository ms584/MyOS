#include "boot.h"
#include "os.h"
#define MIN_HEAP_BYTES (4u*1024u*1024u)
#define MAX_HEAP_BYTES (16u*1024u*1024u)
static size_t heap_bytes;
#define MAX_PAGES (1024u*1024u*1024u/PAGE_SIZE)
#define BOOTSTRAP_PAGES (256u*1024u*1024u/PAGE_SIZE)
#define NX (1ull<<63)
#define ADDR 0x000ffffffffff000ull
/* Keep the BIOS kernel/BSS reservation unchanged. Expand metadata in the
 * kernel heap only after its mappings have been bootstrapped below256MiB. */
static uint8_t bootstrap_available[BOOTSTRAP_PAGES/8],bootstrap_owned[BOOTSTRAP_PAGES/8];
static uint8_t *available=bootstrap_available,*owned=bootstrap_owned;
static unsigned managed_pages=BOOTSTRAP_PAGES;
/* Allocated after heap bootstrap: a static128KiB table would violate the
 * BIOS low-memory kernel reservation. Before then all owned frames have one
 * exclusive reference, and page_retain is unavailable. */
static uint16_t *references;
static size_t free_count,total_count;
/* Bounded allocation history: every successful PMM allocation records its
 * low-water for the current10ms tick.512 slots retain5.12s, allowing an
 * external read-only observer to recover short peaks between polls.
 * No reset/control API and no allocation policy change. */
struct pmm_pressure_sample {uint64_t tick_tag,free_pages;};
static volatile struct pmm_pressure_sample pmm_pressure_history[512];
static void pmm_note_allocation(void){
    uint64_t tag=ticks+1;
    volatile struct pmm_pressure_sample *sample=&pmm_pressure_history[(tag-1)&511];
    if(sample->tick_tag!=tag){sample->free_pages=free_count;sample->tick_tag=tag;}
    else if(free_count<sample->free_pages)sample->free_pages=free_count;
}
static unsigned hint=256;
struct e820 {uint64_t base,length;uint32_t type,attributes;} __attribute__((packed));
static bool bit(uint8_t *b,unsigned n){return (b[n/8]>>(n%8))&1;}
static void set(uint8_t *b,unsigned n,bool v){if(v)b[n/8]|=1u<<(n%8);else b[n/8]&=~(1u<<(n%8));}
size_t pages_free(void){return free_count;}
size_t pages_total(void){return total_count;}
uintptr_t page_alloc(void){
    uint64_t f=irq_save();if(!free_count){irq_restore(f);return 0;}
    for(unsigned k=0;k<managed_pages;++k){unsigned n=(hint+k)%managed_pages;
        if(bit(available,n)&&!bit(owned,n)){set(owned,n,true);if(references)references[n]=1;--free_count;pmm_note_allocation();hint=(n+1)%managed_pages;irq_restore(f);return (uintptr_t)n*PAGE_SIZE;}}
    panic("PMM count mismatch");
}
uintptr_t pages_alloc_contiguous(unsigned count){
    if(!count||count>16)return 0;
    uint64_t flags=irq_save();
    for(unsigned start=256;start+count<=managed_pages;++start){
        bool free=true;
        for(unsigned i=0;i<count;++i)if(!bit(available,start+i)||bit(owned,start+i)){free=false;break;}
        if(!free)continue;
        for(unsigned i=0;i<count;++i){set(owned,start+i,true);if(references)references[start+i]=1;}
        free_count-=count;pmm_note_allocation();irq_restore(flags);return (uintptr_t)start*PAGE_SIZE;
    }
    irq_restore(flags);return 0;
}
void page_free(uintptr_t p){
    uint64_t f=irq_save();unsigned n=p/PAGE_SIZE;
    check(p<(uintptr_t)managed_pages*PAGE_SIZE&&!(p%PAGE_SIZE)&&n<managed_pages&&bit(available,n)&&bit(owned,n),"invalid/double page free");
    if(references)check(references[n]>0,"page ownership reference");
    if(!references||!--references[n]){set(owned,n,false);++free_count;}irq_restore(f);
}
bool page_retain(uintptr_t p){
    uint64_t flags=irq_save();unsigned n=p/PAGE_SIZE;
    bool valid=references&&p<(uintptr_t)managed_pages*PAGE_SIZE&&!(p%PAGE_SIZE)&&
        bit(available,n)&&bit(owned,n)&&references[n]&&references[n]<UINT16_MAX;
    if(valid)++references[n];irq_restore(flags);return valid;
}
unsigned page_references(uintptr_t p){
    uint64_t flags=irq_save();unsigned n=p/PAGE_SIZE;
    unsigned count=p<(uintptr_t)managed_pages*PAGE_SIZE&&!(p%PAGE_SIZE)&&
        bit(available,n)&&bit(owned,n)?(references?references[n]:1):0;
    irq_restore(flags);return count;
}
/* Device BARs can be above4GiB (OVMF places xHCI at0xc000000000).
 * Reserve the last4GiB of the shared supervisor PML4[0] region for MMIO.
 * Ring3 roots share that entry, so timer-driven USB polling stays mapped.
 * Dedicated large pages use UC and NX; RAM and normal vm_map stay separate. */
#define MMIO_BASE 0x7f00000000ull
#define MMIO_PAGES 16u
static unsigned mmio_pages;
uintptr_t vm_map_mmio(uint64_t physical,size_t size){
    if(physical<MAX_PAGES*PAGE_SIZE||!size||size>0x10000||
       physical>0x000fffffffffffffull-size)return 0;
    uint64_t aligned=physical&~0x1fffffull;
    unsigned count=(unsigned)(((physical-aligned)+size+0x1fffff)/0x200000);
    uint64_t flags=irq_save();uintptr_t result=0;
    if(mmio_pages+count>MMIO_PAGES)goto done;
    uint64_t *root=(void *)(uintptr_t)boot_state.root;
    if(!(root[0]&1))goto done;
    uint64_t *pdpt=(void *)(uintptr_t)(root[0]&ADDR);
    unsigned index=MMIO_BASE>>30;
    if(!pdpt[index]){
        uintptr_t table=page_alloc();if(!table)goto done;
        memset((void *)table,0,PAGE_SIZE);pdpt[index]=table|3;
    }
    if((pdpt[index]&0x85)!=1)goto done;
    uint64_t *pd=(void *)(uintptr_t)(pdpt[index]&ADDR);
    for(unsigned i=0;i<count;++i)if(pd[mmio_pages+i])goto done;
    result=MMIO_BASE+(uint64_t)mmio_pages*0x200000;
    for(unsigned i=0;i<count;++i){
        pd[mmio_pages+i]=(aligned+(uint64_t)i*0x200000)|0x9bull|NX;
        uintptr_t address=result+(uintptr_t)i*0x200000;
        __asm__ volatile("invlpg (%0)"::"r"(address):"memory");
    }
    mmio_pages+=count;result+=physical-aligned;
done:irq_restore(flags);return result;
}
static bool virtual_ok(uintptr_t v){return v>=0x100000000ull&&v<MMIO_BASE&&!(v%PAGE_SIZE);}
bool vm_map(uintptr_t va,uintptr_t pa,uint64_t flags){
    if(!virtual_ok(va)||pa%PAGE_SIZE||pa>=(uintptr_t)managed_pages*PAGE_SIZE)return false;
    uint64_t f=irq_save(),*table=(uint64_t *)boot_state.root,*made[3];unsigned count=0;
    for(int shift=39;shift>=21;shift-=9){uint64_t *e=&table[(va>>shift)&511];
        if(!(*e&1)){uintptr_t page=page_alloc();if(!page)goto fail;memset((void *)page,0,PAGE_SIZE);*e=page|3;made[count++]=e;}
        if(*e&128)goto fail;
        table=(uint64_t *)(uintptr_t)(*e&ADDR);
    }
    if(table[(va>>12)&511]&1)goto fail;
    table[(va>>12)&511]=pa|1|(flags&(2|NX));
    __asm__ volatile("invlpg (%0)"::"r"(va):"memory");irq_restore(f);return true;
fail:
    while(count){uint64_t *e=made[--count];page_free(*e&ADDR);*e=0;}irq_restore(f);return false;
}
uintptr_t vm_translate(uintptr_t va){
    uint64_t *table=(uint64_t *)boot_state.root;
    for(int shift=39;shift>=12;shift-=9){uint64_t e=table[(va>>shift)&511];if(!(e&1))return 0;
        if(shift==21&&(e&128))return (e&0x000fffffffe00000ull)|(va&0x1fffff);
        if(shift==12)return (e&ADDR)|(va&4095);
        table=(uint64_t *)(uintptr_t)(e&ADDR);
    }return 0;
}
uintptr_t vm_unmap(uintptr_t va){
    if(!virtual_ok(va))return 0;
    uint64_t f=irq_save(),*table=(uint64_t *)boot_state.root,*parents[3];unsigned n=0;
    for(int shift=39;shift>=21;shift-=9){uint64_t *e=&table[(va>>shift)&511];if(!(*e&1)||(*e&128)){irq_restore(f);return 0;}parents[n++]=e;table=(uint64_t *)(uintptr_t)(*e&ADDR);}
    uint64_t *leaf=&table[(va>>12)&511];uintptr_t pa=*leaf&ADDR;if(!(*leaf&1)){irq_restore(f);return 0;}*leaf=0;
    __asm__ volatile("invlpg (%0)"::"r"(va):"memory");
    while(n){bool empty=true;for(unsigned i=0;i<512;++i)if(table[i]&1){empty=false;break;}if(!empty)break;
        uint64_t *parent=parents[--n];page_free(*parent&ADDR);*parent=0;table=(uint64_t *)((uintptr_t)parent&~4095ull);}
    irq_restore(f);return pa;
}
struct block {size_t size;struct block *next,*prev;uint64_t free;};
static struct block *heap;
/* Kernel allocation high-water includes actual rounded block capacity.
 * Read-only diagnostic symbols: no guest control or budget bypass. */
static size_t heap_live_bytes;
static volatile size_t heap_runtime_peak_bytes;
size_t heap_allocated(void){
    uint64_t flags=irq_save();size_t bytes=0;
    for(struct block *b=heap;b;b=b->next)if(!b->free)bytes+=b->size;
    check(bytes==heap_live_bytes,"heap accounting mismatch");
    irq_restore(flags);return bytes;
}
void *kmalloc(size_t size){
    if(!size||size>heap_bytes-sizeof(struct block))return 0;
    size=(size+15)&~15ull;uint64_t f=irq_save();
    for(struct block *b=heap;b;b=b->next)if(b->free&&b->size>=size){
        if(b->size>=size+sizeof(*b)+16){struct block *n=(struct block *)((char *)(b+1)+size);*n=(struct block){b->size-size-sizeof(*b),b->next,b,1};if(n->next)n->next->prev=n;b->next=n;b->size=size;}
        b->free=0;heap_live_bytes+=b->size;
        if(heap_live_bytes>heap_runtime_peak_bytes)heap_runtime_peak_bytes=heap_live_bytes;
        irq_restore(f);return b+1;
    }irq_restore(f);return 0;
}
void kfree(void *p){
    if(!p)return;uint64_t f=irq_save();struct block *b=heap;while(b&&(void *)(b+1)!=p)b=b->next;
    check(b&&!b->free,"invalid/double heap free");
    check(heap_live_bytes>=b->size,"heap accounting underflow");
    heap_live_bytes-=b->size;b->free=1;
    if(b->next&&b->next->free){struct block *n=b->next;b->size+=sizeof(*n)+n->size;b->next=n->next;if(b->next)b->next->prev=b;}
    if(b->prev&&b->prev->free){struct block *prev=b->prev;prev->size+=sizeof(*b)+b->size;prev->next=b->next;if(b->next)b->next->prev=prev;}
    irq_restore(f);
}
static void discover_pages(unsigned first,unsigned limit){
    uint32_t count=boot_state.count;check(count>0&&count<=256,"boot memory map count");
    const struct e820 *map=(const struct e820 *)boot_state.map;
    for(unsigned pass=0;pass<2;++pass)for(unsigned j=0;j<count;++j){
        const struct e820 *e=&map[j];if(!(e->attributes&1)||!e->length)continue;
        bool usable=e->type==1;if(usable!=(pass==0))continue;
        uint64_t end=e->base+e->length;if(end<e->base)end=UINT64_MAX;
        for(unsigned n=first;n<limit;++n){uint64_t start=(uint64_t)n*PAGE_SIZE;
            if(usable?(start>=e->base&&start+PAGE_SIZE<=end):(start<end&&start+PAGE_SIZE>e->base))set(available,n,usable);
        }
    }
}
void memory_init(void){
    discover_pages(256,managed_pages);
    for(unsigned n=0;n<managed_pages;++n)if(bit(available,n))++free_count;
    total_count=free_count; /* Managed usable pages, before heap allocations. */
    heap_bytes=free_count>MAX_HEAP_BYTES/PAGE_SIZE+2048?MAX_HEAP_BYTES:MIN_HEAP_BYTES;
    check(free_count>heap_bytes/PAGE_SIZE+512,"insufficient RAM");
    for(unsigned i=0;i<heap_bytes/PAGE_SIZE;++i){uintptr_t p=page_alloc();check(p&&vm_map(0x100000000ull+i*PAGE_SIZE,p,2|NX),"heap mapping");}
    heap=(struct block *)0x100000000ull;*heap=(struct block){heap_bytes-sizeof(*heap),0,0,1};
    uint8_t *expanded_available=kmalloc(MAX_PAGES/8),*expanded_owned=kmalloc(MAX_PAGES/8);
    check(expanded_available&&expanded_owned,"PMM bitmap metadata");
    memset(expanded_available,0,MAX_PAGES/8);memset(expanded_owned,0,MAX_PAGES/8);
    memcpy(expanded_available,available,BOOTSTRAP_PAGES/8);
    memcpy(expanded_owned,owned,BOOTSTRAP_PAGES/8);
    available=expanded_available;owned=expanded_owned;managed_pages=MAX_PAGES;
    discover_pages(BOOTSTRAP_PAGES,MAX_PAGES);
    for(unsigned n=BOOTSTRAP_PAGES;n<MAX_PAGES;++n)if(bit(available,n)){++free_count;++total_count;}
    references=kmalloc(MAX_PAGES*sizeof(*references));check(references!=0,"PMM reference metadata");
    for(unsigned n=0;n<MAX_PAGES;++n)references[n]=bit(owned,n)?1:0;
}
void memory_test(void){
    size_t heap_before=heap_allocated();
    size_t before=pages_free();uintptr_t a=page_alloc(),b=page_alloc();check(a&&b&&a!=b&&a>=0x100000,"PMM allocation");
    check(page_references(a)==1&&page_retain(a)&&page_references(a)==2,"PMM shared retain");
    page_free(a);check(page_references(a)==1&&pages_free()==before-2,"PMM retained frame lifetime");
    check(!page_retain(a+1)&&!page_references(a+1)&&!page_retain(0),"PMM invalid retain");
    uintptr_t va=0x200000000ull;check(vm_map(va,a,2|NX),"VM map");check(!vm_map(va,b,2|NX),"duplicate map");
    *(volatile uint64_t *)va=0x1122334455667788ull;check(*(volatile uint64_t *)a==0x1122334455667788ull,"VM alias");
    check(vm_translate(va+17)==a+17,"VM translation");check(vm_unmap(va)==a&&!vm_translate(va),"VM unmap");page_free(a);page_free(b);check(pages_free()==before,"page table reclaim");
    void *x=kmalloc(13),*y=kmalloc(5000);check(x&&y&&x!=y&&!((uintptr_t)x&15),"heap allocation");
    memset(x,0xab,13);memset(y,0xcd,5000);check(((uint8_t *)x)[12]==0xab&&((uint8_t *)y)[4999]==0xcd,"heap boundaries");
    kfree(x);kfree(y);size_t largest=0;for(struct block *b=heap;b;b=b->next)if(b->free&&b->size>largest)largest=b->size;
    void *all=kmalloc(largest);check(all==x,"heap coalescing");check(!kmalloc(1),"heap OOM");kfree(all);
    /* Reserve page-table frames before exhaustion, then prove a frame above
     * the former256MiB cap is writable, mapped, retained and reclaimed. */
    uintptr_t high=0;check(vm_map(va,0x100000,2|NX),"PMM high probe table");
    uintptr_t list=0,p;size_t used=0;while((p=page_alloc())){*(uintptr_t *)p=list;list=p;++used;if(p>=256u*1024u*1024u)high=p;}
    check(vm_unmap(va)==0x100000,"PMM probe unmap");
    /* vm_unmap reclaimed the preallocated tables; count those separately. */
    if(high){check(page_retain(high)&&page_references(high)==2,"PMM high retain");
        check(vm_map(va,high,2|NX),"PMM high map");
        *(volatile uint64_t *)(va+16)=0x512512abcdef1234ull;
        check(*(volatile uint64_t *)(high+16)==0x512512abcdef1234ull&&vm_translate(va+16)==high+16,"PMM high alias");
        check(vm_unmap(va)==high,"PMM high unmap");page_free(high);check(page_references(high)==1,"PMM high release");
        print("PASS: PMM frames above256MiB, alias and retained lifetime\n");
        if(high>=512u*1024u*1024u)print("PASS: PMM frames above512MiB, alias and retained lifetime\n");}
    /* The probe tables are free now; include them in the exhaustion list. */
    while((p=page_alloc())){*(uintptr_t *)p=list;list=p;++used;}
    check(used==before&&!page_alloc(),"PMM exhaustion");while(list){p=*(uintptr_t *)list;page_free(list);list=p;}check(pages_free()==before,"PMM recovery");
    check(heap_allocated()==heap_before,"heap accounting recovery");
    /* Start runtime high-water only after deliberate whole-heap boot OOM.
     * It still includes every later kernel allocation, not only media. */
    heap_runtime_peak_bytes=heap_live_bytes;
    print("PASS: PMM, paging, heap, exhaustion/recovery\n");
}
