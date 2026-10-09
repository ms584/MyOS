#ifndef MYOS_VM_AREA_H
#define MYOS_VM_AREA_H
#include <stdint.h>
#include <stdbool.h>
#define VM_AREA_LIMIT 1024u
struct vm_area { uint64_t begin,end; };
struct vm_areas { unsigned count;struct vm_area area[VM_AREA_LIMIT]; };
/* Sorted disjoint, non-adjacent half-open intervals. Ownership only;
 * resident page permissions remain in PTEs. All callers serialize mutations. */
static inline bool vm_areas_cover(const struct vm_areas *v,uint64_t a,uint64_t b)
{
    if(!v||a>=b)return false;
    for(unsigned i=0;i<v->count;++i)
        if(v->area[i].begin<=a&&v->area[i].end>=b)return true;
    return false;
}
static inline bool vm_areas_overlap(const struct vm_areas *v,uint64_t a,uint64_t b)
{
    if(!v)return false;
    for(unsigned i=0;i<v->count;++i)
        if(v->area[i].begin<b&&a<v->area[i].end)return true;
    return false;
}
static inline bool vm_areas_add(struct vm_areas *v,uint64_t a,uint64_t b)
{
    if(a>=b||vm_areas_overlap(v,a,b))return false;
    unsigned i=0;while(i<v->count&&v->area[i].begin<a)++i;
    if(i&&v->area[i-1].end==a){
        v->area[i-1].end=b;
        if(i<v->count&&v->area[i].begin==b){
            v->area[i-1].end=v->area[i].end;
            for(unsigned j=i;j+1<v->count;++j)v->area[j]=v->area[j+1];--v->count;
        }
    }else if(i<v->count&&v->area[i].begin==b)v->area[i].begin=a;
    else {
        if(v->count==VM_AREA_LIMIT)return false;
        for(unsigned j=v->count;j>i;--j)v->area[j]=v->area[j-1];
        v->area[i]=(struct vm_area){a,b};++v->count;
    }
    return true;
}
static inline bool vm_areas_remove(struct vm_areas *v,uint64_t a,uint64_t b)
{
    if(a>=b)return false;if(!v)return true;
    /* A single removal can split at most one existing interval. Preflight
     * capacity before mutation, including the full-table failure case. */
    for(unsigned i=0;i<v->count;++i)
        if(a>v->area[i].begin&&b<v->area[i].end&&v->count==VM_AREA_LIMIT)return false;
    for(unsigned i=0;i<v->count;){
        struct vm_area old=v->area[i];
        if(old.end<=a||old.begin>=b){++i;continue;}
        if(a>old.begin&&b<old.end){
            for(unsigned j=v->count;j>i+1;--j)v->area[j]=v->area[j-1];
            v->area[i].end=a;v->area[i+1]=(struct vm_area){b,old.end};++v->count;break;
        }
        if(a>old.begin){v->area[i].end=a;++i;}
        else if(b<old.end){v->area[i].begin=b;++i;}
        else {for(unsigned j=i;j+1<v->count;++j)v->area[j]=v->area[j+1];--v->count;}
    }
    return true;
}
static inline uint64_t vm_areas_gap(const struct vm_areas *v,uint64_t begin,uint64_t end,uint64_t length)
{
    if(!length||begin>=end||length>end-begin)return 0;
    uint64_t candidate=begin;
    if(v)for(unsigned i=0;i<v->count;++i){
        struct vm_area r=v->area[i];if(r.end<=candidate)continue;
        if(r.begin>=end)break;
        if(r.begin>candidate&&length<=r.begin-candidate)return candidate;
        if(r.end>candidate)candidate=r.end;
        if(candidate>end||length>end-candidate)return 0;
    }
    return candidate<=end&&length<=end-candidate?candidate:0;
}
#endif
