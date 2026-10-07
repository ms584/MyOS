#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record-lock.h"
static unsigned heaps;
static bool deny;
void *kmalloc(size_t n){if(deny)return 0;void *p=malloc(n);if(p)++heaps;return p;}
void kfree(void *p){if(p){assert(heaps);--heaps;free(p);}}
static int set(uintptr_t owner,int type,uint64_t start,uint64_t end)
{return record_apply(1,owner,(int32_t)(100+owner),RECORD_SET,type,start,end,0);}
static struct record_lock_abi query(uintptr_t owner,int type,uint64_t start,uint64_t end)
{
 struct record_lock_abi result={7,9,0xa5a5a5a5,4,5,6,0xa5a5a5a5};
 assert(!record_apply(1,owner,(int32_t)(100+owner),RECORD_GET,type,start,end,&result));return result;
}
int main(void)
{
 deny=true;assert(set(1,RECORD_WRITE,0,UINT64_C(1)<<63)==-12&&!heaps);deny=false;
 assert(!set(1,RECORD_WRITE,0,UINT64_C(1)<<63)&&heaps==1);
 assert(set(2,RECORD_READ,10,11)==-11);
 struct record_lock_abi found=query(2,RECORD_READ,10,11);
 assert(found.type==RECORD_WRITE&&found.whence==0&&found.start==0&&found.length==0&&found.pid==101);
 assert(!set(1,RECORD_UNLOCK,10,20)&&!set(2,RECORD_WRITE,10,20));
 assert(query(3,RECORD_READ,10,20).pid==102&&query(3,RECORD_READ,20,21).pid==101);
 record_release(1,1);assert(query(1,RECORD_READ,0,1).type==RECORD_UNLOCK&&heaps==1);
 record_release(1,2);assert(!heaps);
 assert(!set(1,RECORD_READ,0,10)&&!set(2,RECORD_READ,0,10));
 assert(set(1,RECORD_WRITE,3,7)==-11);record_release(1,2);
 assert(!set(1,RECORD_WRITE,3,7)&&query(2,RECORD_WRITE,1,2).type==RECORD_READ&&
        query(2,RECORD_READ,3,7).type==RECORD_WRITE&&query(2,RECORD_WRITE,8,9).type==RECORD_READ);
 assert(!set(1,RECORD_WRITE,0,10));record_release(1,1);assert(!heaps);
 /* Capacity failure including unlock splitting is atomic, not partial. */
 assert(!set(1,RECORD_WRITE,0,3));
 for(unsigned i=1;i<RECORD_LIMIT;++i)assert(!set(1,RECORD_WRITE,10+4*i,12+4*i));
 assert(set(1,RECORD_WRITE,10000,10001)==-37&&set(1,RECORD_UNLOCK,1,2)==-37);
 found=query(2,RECORD_READ,1,2);assert(found.start==0&&found.length==3);
 record_release(1,1);assert(!heaps);
 assert(!set(1,RECORD_WRITE,INT64_MAX,UINT64_C(1)<<63));
 found=query(2,RECORD_READ,INT64_MAX,UINT64_C(1)<<63);
 assert(found.start==INT64_MAX&&found.length==0);record_release(1,1);
 assert(record_apply(1,1,101,RECORD_SET_WAIT,RECORD_WRITE,0,1,0)==-95&&!heaps);
 assert(set(1,3,0,1)==-22&&set(1,RECORD_WRITE,1,1)==-22&&!heaps);
 /* Independent byte-range oracle across four owners, including conversions,
  * coalescing, overlap denial, partial unlock, and GETLK conflict ownership. */
 unsigned char model[4][64]={{0}};uint32_t seed=0x13572468;
 for(unsigned turn=0;turn<2000;++turn){
  seed=seed*1664525u+1013904223u;unsigned owner=(seed>>28)%4;
  unsigned begin=(seed>>8)%64,end=begin+1+(seed% (64-begin)),type=(seed>>20)%3;
  bool conflict=false;if(type!=RECORD_UNLOCK)for(unsigned o=0;o<4;++o)if(o!=owner)
   for(unsigned b=begin;b<end;++b)if(model[o][b]&&(type==RECORD_WRITE||model[o][b]==2))conflict=true;
  int rc=set(owner+1,(int)type,begin,end);
  if(conflict)assert(rc==-11);else {assert(rc==0||rc==-37);
   if(!rc)for(unsigned b=begin;b<end;++b)model[owner][b]=type==RECORD_UNLOCK?0:(unsigned char)(type+1);}
  for(unsigned o=0;o<4;++o)for(unsigned b=0;b<64;++b){
   bool exists=false;for(unsigned other=0;other<4;++other)if(other!=o&&model[other][b])exists=true;
   found=query(o+1,RECORD_WRITE,b,b+1);
   if(!exists)assert(found.type==RECORD_UNLOCK);
   else {assert(found.pid>=101&&found.pid<=104&&(unsigned)(found.pid-101)!=o);
    unsigned other=(unsigned)(found.pid-101);assert(model[other][b]==found.type+1&&
     found.start<=(int64_t)b&&found.length>0&&(uint64_t)(found.start+found.length)>b);}
  }
 }
 for(unsigned o=1;o<=4;++o)record_release(1,o);assert(!heaps);
 puts("PASS real record-lock engine: conflict ownership, read sharing, split/coalesce/conversion/EOF, atomic ENOLCK/OOM, 2000-step independent range oracle, exact pool teardown; NOT kernel/runtime locking proof");
}
