#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vm-area.h"
static void invariants(struct vm_areas *v)
{
    assert(v->count<=VM_AREA_LIMIT);
    for(unsigned i=0;i<v->count;++i){assert(v->area[i].begin<v->area[i].end);
        if(i)assert(v->area[i-1].end<v->area[i].begin);}
}
int main(void)
{
    struct vm_areas v={0};
    assert(vm_areas_add(&v,0x100000000ull,0x900000000ull));
    assert(v.count==1&&vm_areas_cover(&v,0x200000000ull,0x800000000ull));
    assert(!vm_areas_add(&v,0x100000000ull,0x200000000ull));
    assert(vm_areas_remove(&v,0x400000000ull,0x400001000ull));
    assert(v.count==2&&!vm_areas_cover(&v,0x400000000ull,0x400001000ull));
    assert(vm_areas_add(&v,0x400000000ull,0x400001000ull)&&v.count==1);
    assert(vm_areas_remove(&v,0,UINT64_MAX)&&!v.count);
    /* Model-check 100000 insert/remove operations against exact occupancy. */
    unsigned char occupied[128]={0};uint32_t seed=7349;
    for(unsigned trial=0;trial<100000;++trial){
        seed=seed*1664525+1013904223;unsigned a=seed%128;
        seed=seed*1664525+1013904223;unsigned b=a+1+seed%(128-a);
        seed=seed*1664525+1013904223;
        if(seed&1){bool overlap=false;for(unsigned i=a;i<b;++i)overlap|=occupied[i]!=0;
            assert(vm_areas_add(&v,4096ull*(a+1),4096ull*(b+1))==!overlap);
            if(!overlap)for(unsigned i=a;i<b;++i)occupied[i]=1;
        }else{assert(vm_areas_remove(&v,4096ull*(a+1),4096ull*(b+1)));
            for(unsigned i=a;i<b;++i)occupied[i]=0;}
        invariants(&v);
        for(unsigned i=0;i<128;++i)assert(vm_areas_cover(&v,4096ull*(i+1),4096ull*(i+2))==(occupied[i]!=0));
        unsigned run=0;uint64_t gap=0;
        for(unsigned i=0;i<128;++i){run=occupied[i]?0:run+1;if(run==3){gap=4096ull*(i-1);break;}}
        assert(vm_areas_gap(&v,4096,129*4096,3*4096)==gap);
    }
    memset(&v,0,sizeof(v));
    for(unsigned i=0;i<VM_AREA_LIMIT;++i)assert(vm_areas_add(&v,4ull*i+1,4ull*i+4));
    struct vm_areas before=v;
    assert(!vm_areas_add(&v,2000,2001)&&!memcmp(&before,&v,sizeof(v)));
    assert(!vm_areas_remove(&v,2,3)&&!memcmp(&before,&v,sizeof(v)));
    assert(vm_areas_add(&v,4,5)&&v.count==VM_AREA_LIMIT-1);
    assert(vm_areas_remove(&v,4,5)&&v.count==VM_AREA_LIMIT);
    assert(!vm_areas_gap(&v,UINT64_MAX-2,UINT64_MAX,3));
    puts("PASS vm-area exact ownership/split/merge/full-table rollback/100000 model operations");
}
