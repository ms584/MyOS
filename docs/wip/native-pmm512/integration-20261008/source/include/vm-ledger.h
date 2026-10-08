#ifndef MYOS_VM_LEDGER_H
#define MYOS_VM_LEDGER_H
#include "process.h"
/* One allocation: dense page ledger, capacity link slots, 2*capacity buckets.
 * Link values are dense indices +1; zero is the end sentinel. Shared frame
 * aliases may appear more than once: this is an indexed multiset, not a set.
 * Caller holds the existing VM operation's interrupt exclusion. */
static unsigned *vm_ledger_links(struct process *p)
{return (unsigned *)(p->vm_pages+p->vm_page_capacity);}
static unsigned *vm_ledger_buckets(struct process *p)
{return vm_ledger_links(p)+p->vm_page_capacity;}
static unsigned vm_ledger_bucket(uintptr_t page,unsigned capacity)
{
    uintptr_t key=page>>12;key^=key>>19;
    return (unsigned)(key*0x9e3779b1u)&(2u*capacity-1u);
}
static bool vm_track(struct process *p,uintptr_t page)
{
    if(p->vm_page_count==p->vm_page_capacity){
        if(p->vm_page_capacity>UINT32_MAX/4u)return false;
        unsigned capacity=p->vm_page_capacity?p->vm_page_capacity*2u:32u;
        uintptr_t *pages=kmalloc((size_t)capacity*(sizeof(*pages)+3u*sizeof(unsigned)));
        if(!pages)return false;
        unsigned *links=(unsigned *)(pages+capacity),*buckets=links+capacity;
        memset(links,0,(size_t)capacity*3u*sizeof(*links));
        if(p->vm_pages)memcpy(pages,p->vm_pages,(size_t)p->vm_page_count*sizeof(*pages));
        for(unsigned i=0;i<p->vm_page_count;++i){
            unsigned bucket=vm_ledger_bucket(pages[i],capacity);
            links[i]=buckets[bucket];buckets[bucket]=i+1u;
        }
        /* Nothing published until the complete replacement index exists. */
        kfree(p->vm_pages);p->vm_pages=pages;p->vm_page_capacity=capacity;
    }
    unsigned index=p->vm_page_count++,bucket=vm_ledger_bucket(page,p->vm_page_capacity);
    unsigned *links=vm_ledger_links(p),*buckets=vm_ledger_buckets(p);
    p->vm_pages[index]=page;links[index]=buckets[bucket];buckets[bucket]=index+1u;
    return true;
}
static unsigned *vm_ledger_find(struct process *p,uintptr_t page)
{
    if(!p->vm_page_capacity)return 0;
    unsigned *links=vm_ledger_links(p);
    unsigned *link=&vm_ledger_buckets(p)[vm_ledger_bucket(page,p->vm_page_capacity)];
    while(*link&&p->vm_pages[*link-1u]!=page)link=&links[*link-1u];
    return link;
}
static bool vm_owned(struct process *p,uintptr_t page)
{unsigned *link=vm_ledger_find(p,page);return link&&*link;}
static void vm_forget(struct process *p,uintptr_t page)
{
    unsigned *link=vm_ledger_find(p,page);
    if(!link||!*link)panic("untracked process VM page");
    unsigned index=*link-1u,*links=vm_ledger_links(p);
    *link=links[index];unsigned last=--p->vm_page_count;
    if(index!=last){
        /* Find by dense index, not by physical page: duplicates are valid. */
        unsigned bucket=vm_ledger_bucket(p->vm_pages[last],p->vm_page_capacity);
        unsigned *moved=&vm_ledger_buckets(p)[bucket];
        while(*moved&&*moved!=last+1u)moved=&links[*moved-1u];
        if(!*moved)panic("untracked moved process VM page");
        *moved=index+1u;p->vm_pages[index]=p->vm_pages[last];links[index]=links[last];
    }
    p->vm_pages[last]=0;links[last]=0;
}
#endif
