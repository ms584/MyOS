#include "record-lock.h"
struct record {
    uint64_t key,start,end;uintptr_t owner;int32_t pid;uint32_t type;
};
struct record_state {
    unsigned count;
    struct record live[RECORD_LIMIT],scratch[RECORD_LIMIT*2+1];
};
/* No permanent allocation or expanded BIOS BSS. Last lock release frees the
 * whole bounded pool; split/coalesce happens in scratch before atomic commit. */
static struct record_state *state;
static bool less(struct record a,struct record b)
{
    if(a.key!=b.key)return a.key<b.key;
    if(a.owner!=b.owner)return a.owner<b.owner;
    if(a.type!=b.type)return a.type<b.type;
    return a.start<b.start;
}
static bool conflicts(struct record r,uint64_t key,uintptr_t owner,int type,uint64_t start,uint64_t end)
{return r.key==key&&r.owner!=owner&&r.start<end&&start<r.end&&
        (type==RECORD_WRITE||r.type==RECORD_WRITE);}
int record_apply(uint64_t key,uintptr_t owner,int32_t pid,unsigned command,int type,
                 uint64_t start,uint64_t end,struct record_lock_abi *result)
{
    if(!key||!owner||pid<=0||start>=end||end>(UINT64_C(1)<<63)||type<0||type>RECORD_UNLOCK)return -22;
    if(command!=RECORD_GET&&command!=RECORD_SET)return command==RECORD_SET_WAIT?-95:-22;
    if(command==RECORD_GET&&(type==RECORD_UNLOCK||!result))return -22;
    const struct record *conflict=0;
    if(type!=RECORD_UNLOCK&&state)for(unsigned i=0;i<state->count;++i){
        const struct record *r=&state->live[i];
        if(conflicts(*r,key,owner,type,start,end)&&
           (!conflict||r->start<conflict->start||
            (r->start==conflict->start&&r->pid<conflict->pid)))conflict=r;
    }
    if(command==RECORD_GET){
        if(!conflict)result->type=RECORD_UNLOCK;
        else *result=(struct record_lock_abi){(int16_t)conflict->type,0,0,
            (int64_t)conflict->start,conflict->end==(UINT64_C(1)<<63)?0:
            (int64_t)(conflict->end-conflict->start),conflict->pid,0};
        return 0;
    }
    if(conflict)return -11;
    if(!state&&type==RECORD_UNLOCK)return 0;
    bool created=!state;struct record_state *next=state;
    if(created){next=kmalloc(sizeof(*next));if(!next)return -12;memset(next,0,sizeof(*next));}
    unsigned count=0;
    for(unsigned i=0;i<next->count;++i){struct record r=next->live[i];
        if(r.key!=key||r.owner!=owner||r.end<=start||r.start>=end){next->scratch[count++]=r;continue;}
        if(r.start<start){struct record left=r;left.end=start;next->scratch[count++]=left;}
        if(r.end>end){r.start=end;next->scratch[count++]=r;}
    }
    if(type!=RECORD_UNLOCK)next->scratch[count++]=(struct record){key,start,end,owner,pid,(uint32_t)type};
    for(unsigned i=1;i<count;++i){struct record r=next->scratch[i];unsigned j=i;
        while(j&&less(r,next->scratch[j-1])){next->scratch[j]=next->scratch[j-1];--j;}
        next->scratch[j]=r;
    }
    unsigned merged=0;
    for(unsigned i=0;i<count;++i){struct record r=next->scratch[i];
        struct record *previous=merged?&next->scratch[merged-1]:0;
        if(previous&&previous->key==r.key&&previous->owner==r.owner&&previous->type==r.type&&r.start<=previous->end){
            if(r.end>previous->end)previous->end=r.end;
        }else next->scratch[merged++]=r;
    }
    if(merged>RECORD_LIMIT){if(created)kfree(next);return -37;}
    memcpy(next->live,next->scratch,merged*sizeof(next->live[0]));next->count=merged;state=next;
    if(!merged){state=0;kfree(next);}return 0;
}
void record_release(uint64_t key,uintptr_t owner)
{
    if(!state)return;
    unsigned count=0;
    for(unsigned i=0;i<state->count;++i){struct record r=state->live[i];
        if(r.key!=key||r.owner!=owner)state->live[count++]=r;
    }
    state->count=count;if(!count){struct record_state *old=state;state=0;kfree(old);}
}
