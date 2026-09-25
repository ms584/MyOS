#include "boot.h"
#include "os.h"
#define MIN_HEAP_BYTES (4u*1024u*1024u)
#define MAX_HEAP_BYTES (16u*1024u*1024u)
static size_t heap_bytes;
#define MAX_PAGES (256u*1024u*1024u/PAGE_SIZE)
#define NX (1ull<<63)
#define ADDR 0x000ffffffffff000ull
static uint8_t available[MAX_PAGES/8],owned[MAX_PAGES/8];
static size_t free_count;
static unsigned hint=256;
struct e820 {uint64_t base,length;uint32_t type,attributes;} __attribute__((packed));
static bool bit(uint8_t *b,unsigned n){return (b[n/8]>>(n%8))&1;}
static void set(uint8_t *b,unsigned n,bool v){if(v)b[n/8]|=1u<<(n%8);else b[n/8]&=~(1u<<(n%8));}
size_t pages_free(void){return free_count;}
uintptr_t page_alloc(void){
    uint64_t f=irq_save();if(!free_count){irq_restore(f);return 0;}
    for(unsigned k=0;k<MAX_PAGES;++k){unsigned n=(hint+k)%MAX_PAGES;
        if(bit(available,n)&&!bit(owned,n)){set(owned,n,true);--free_count;hint=(n+1)%MAX_PAGES;irq_restore(f);return (uintptr_t)n*PAGE_SIZE;}}
    panic("PMM count mismatch");
}
void page_free(uintptr_t p){
    uint64_t f=irq_save();unsigned n=p/PAGE_SIZE;
    check(p<(uintptr_t)MAX_PAGES*PAGE_SIZE&&!(p%PAGE_SIZE)&&n<MAX_PAGES&&bit(available,n)&&bit(owned,n),"invalid/double page free");
    set(owned,n,false);++free_count;irq_restore(f);
}
static bool virtual_ok(uintptr_t v){return v>=0x100000000ull&&v<(1ull<<39)&&!(v%PAGE_SIZE);}
bool vm_map(uintptr_t va,uintptr_t pa,uint64_t flags){
    if(!virtual_ok(va)||pa%PAGE_SIZE||pa>=MAX_PAGES*PAGE_SIZE)return false;
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
void *kmalloc(size_t size){
    if(!size||size>heap_bytes-sizeof(struct block))return 0;
    size=(size+15)&~15ull;uint64_t f=irq_save();
    for(struct block *b=heap;b;b=b->next)if(b->free&&b->size>=size){
        if(b->size>=size+sizeof(*b)+16){struct block *n=(struct block *)((char *)(b+1)+size);*n=(struct block){b->size-size-sizeof(*b),b->next,b,1};if(n->next)n->next->prev=n;b->next=n;b->size=size;}
        b->free=0;irq_restore(f);return b+1;
    }irq_restore(f);return 0;
}
void kfree(void *p){
    if(!p)return;uint64_t f=irq_save();struct block *b=heap;while(b&&(void *)(b+1)!=p)b=b->next;
    check(b&&!b->free,"invalid/double heap free");b->free=1;
    if(b->next&&b->next->free){struct block *n=b->next;b->size+=sizeof(*n)+n->size;b->next=n->next;if(b->next)b->next->prev=b;}
    if(b->prev&&b->prev->free){struct block *prev=b->prev;prev->size+=sizeof(*b)+b->size;prev->next=b->next;if(b->next)b->next->prev=prev;}
    irq_restore(f);
}
void memory_init(void){
    uint32_t count=boot_state.count;check(count>0&&count<=256,"boot memory map count");
    const struct e820 *map=(const struct e820 *)boot_state.map;
    for(unsigned pass=0;pass<2;++pass)for(unsigned j=0;j<count;++j){
        const struct e820 *e=&map[j];if(!(e->attributes&1)||!e->length)continue;
        bool usable=e->type==1;if(usable!=(pass==0))continue;
        uint64_t end=e->base+e->length;if(end<e->base)end=UINT64_MAX;
        for(unsigned n=256;n<MAX_PAGES;++n){uint64_t start=(uint64_t)n*PAGE_SIZE;
            if(usable?(start>=e->base&&start+PAGE_SIZE<=end):(start<end&&start+PAGE_SIZE>e->base))set(available,n,usable);
        }
    }
    for(unsigned n=0;n<MAX_PAGES;++n)if(bit(available,n))++free_count;
    heap_bytes=free_count>MAX_HEAP_BYTES/PAGE_SIZE+2048?MAX_HEAP_BYTES:MIN_HEAP_BYTES;
    check(free_count>heap_bytes/PAGE_SIZE+512,"insufficient RAM");
    for(unsigned i=0;i<heap_bytes/PAGE_SIZE;++i){uintptr_t p=page_alloc();check(p&&vm_map(0x100000000ull+i*PAGE_SIZE,p,2|NX),"heap mapping");}
    heap=(struct block *)0x100000000ull;*heap=(struct block){heap_bytes-sizeof(*heap),0,0,1};
}
void memory_test(void){
    size_t before=pages_free();uintptr_t a=page_alloc(),b=page_alloc();check(a&&b&&a!=b&&a>=0x100000,"PMM allocation");
    uintptr_t va=0x200000000ull;check(vm_map(va,a,2|NX),"VM map");check(!vm_map(va,b,2|NX),"duplicate map");
    *(volatile uint64_t *)va=0x1122334455667788ull;check(*(volatile uint64_t *)a==0x1122334455667788ull,"VM alias");
    check(vm_translate(va+17)==a+17,"VM translation");check(vm_unmap(va)==a&&!vm_translate(va),"VM unmap");page_free(a);page_free(b);check(pages_free()==before,"page table reclaim");
    void *x=kmalloc(13),*y=kmalloc(5000);check(x&&y&&x!=y&&!((uintptr_t)x&15),"heap allocation");
    memset(x,0xab,13);memset(y,0xcd,5000);check(((uint8_t *)x)[12]==0xab&&((uint8_t *)y)[4999]==0xcd,"heap boundaries");
    kfree(x);kfree(y);void *all=kmalloc(heap_bytes-sizeof(struct block));check(all==x,"heap coalescing");check(!kmalloc(1),"heap OOM");kfree(all);
    uintptr_t list=0,p;size_t used=0;while((p=page_alloc())){*(uintptr_t *)p=list;list=p;++used;}
    check(used==before&&!page_alloc(),"PMM exhaustion");while(list){p=*(uintptr_t *)list;page_free(list);list=p;}check(pages_free()==before,"PMM recovery");
    print("PASS: PMM, paging, heap, exhaustion/recovery\n");
}
