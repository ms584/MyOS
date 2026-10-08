#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define putchar myos_kernel_putchar
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include <process.h>
#pragma GCC diagnostic pop
#undef putchar
static bool fail_next;
static size_t heap_bytes;
void *kmalloc(size_t size){
    if(fail_next){fail_next=false;return 0;}
    size_t *p=malloc(size+sizeof(*p));assert(p);*p=size;heap_bytes+=size;return p+1;
}
void kfree(void *pointer){if(pointer){size_t *p=(size_t *)pointer-1;heap_bytes-=*p;free(p);}}
_Noreturn void panic(const char *message){fprintf(stderr,"unexpected panic: %s\n",message);abort();}
#include "vm-ledger.h"
static void invariants(struct process *p){
    if(!p->vm_page_capacity){assert(!p->vm_pages&&!p->vm_page_count);return;}
    assert(p->vm_page_count<=p->vm_page_capacity);
    unsigned char *seen=calloc(p->vm_page_capacity,1);assert(seen);
    unsigned found=0,*links=vm_ledger_links(p),*buckets=vm_ledger_buckets(p);
    for(unsigned b=0;b<2*p->vm_page_capacity;++b){
        unsigned steps=0;
        for(unsigned link=buckets[b];link;link=links[link-1]){
            assert(link<=p->vm_page_count&&++steps<=p->vm_page_count);
            unsigned index=link-1;assert(!seen[index]);seen[index]=1;
            assert(vm_ledger_bucket(p->vm_pages[index],p->vm_page_capacity)==b);
            assert(vm_owned(p,p->vm_pages[index]));++found;
        }
    }
    assert(found==p->vm_page_count);free(seen);
}
int main(void){
    struct process p={0};fail_next=true;
    assert(!vm_track(&p,4096));invariants(&p);assert(!heap_bytes);
    /* Deliberate collisions, duplicates and swap-removal from bucket chains. */
    for(unsigned i=0;i<32;++i)assert(vm_track(&p,(uintptr_t)(1+(i%16)*64)*4096));
    invariants(&p);uintptr_t *old=p.vm_pages;unsigned capacity=p.vm_page_capacity;
    size_t bytes=heap_bytes;unsigned char *copy=malloc(bytes);assert(copy);memcpy(copy,old,bytes);
    fail_next=true;assert(!vm_track(&p,0x12345000));
    assert(p.vm_pages==old&&p.vm_page_capacity==capacity&&p.vm_page_count==32&&heap_bytes==bytes);
    assert(!memcmp(copy,old,bytes));free(copy);invariants(&p);
    assert(vm_track(&p,0x12345000));vm_forget(&p,0x12345000);
    for(unsigned round=0;round<2;++round)for(unsigned i=0;i<16;++i){
        uintptr_t page=(uintptr_t)(1+i*64)*4096;assert(vm_owned(&p,page));vm_forget(&p,page);
        assert(vm_owned(&p,page)==(round==0));invariants(&p);
    }
    assert(!p.vm_page_count);kfree(p.vm_pages);memset(&p,0,sizeof(p));assert(!heap_bytes);
    unsigned counts[128]={0},total=0;uint32_t seed=7349;
    for(unsigned trial=0;trial<20000;++trial){
        seed=seed*1664525u+1013904223u;unsigned key=(seed>>16)%128;
        uintptr_t page=(uintptr_t)(1+key)*4096;
        if(!counts[key]||((seed&1)&&total<1024)){assert(vm_track(&p,page));++counts[key];++total;}
        else{vm_forget(&p,page);--counts[key];--total;}
        assert(p.vm_page_count==total);
        for(unsigned i=0;i<128;++i)assert(vm_owned(&p,(uintptr_t)(1+i)*4096)==(counts[i]!=0));
        if(!(trial%71))invariants(&p);
    }
    for(unsigned key=0;key<128;++key)while(counts[key]){
        vm_forget(&p,(uintptr_t)(1+key)*4096);--counts[key];
    }
    invariants(&p);assert(!p.vm_page_count);kfree(p.vm_pages);memset(&p,0,sizeof(p));
    /* Native-scale dense ownership and reverse removal without a full scan. */
    for(unsigned i=0;i<65536;++i)assert(vm_track(&p,(uintptr_t)(i+1)*4096));
    invariants(&p);
    for(unsigned i=0;i<65536;++i){assert(vm_owned(&p,(uintptr_t)(i+1)*4096));vm_forget(&p,(uintptr_t)(i+1)*4096);}
    assert(!p.vm_page_count);invariants(&p);kfree(p.vm_pages);assert(!heap_bytes);
    puts("PASS VM indexed multiset: collisions/duplicates/swap removal/20000 model operations/growth OOM rollback/65536 pages/no heap leak");
}
