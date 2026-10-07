#include "shm-backing.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct frame {uintptr_t address;unsigned refs;};
static struct frame frames[512];
static int allocations_before_failure=-1;
static unsigned live;
uintptr_t page_alloc(void)
{
 if(!allocations_before_failure)return 0;
 if(allocations_before_failure>0)--allocations_before_failure;
 for(unsigned i=0;i<512;++i)if(!frames[i].refs){
  void *page=aligned_alloc(PAGE_SIZE,PAGE_SIZE);assert(page);
  frames[i]=(struct frame){(uintptr_t)page,1};++live;return (uintptr_t)page;
 }return 0;
}
unsigned page_references(uintptr_t page)
{for(unsigned i=0;i<512;++i)if(frames[i].address==page&&frames[i].refs)return frames[i].refs;return 0;}
bool page_retain(uintptr_t page)
{for(unsigned i=0;i<512;++i)if(frames[i].address==page&&frames[i].refs){++frames[i].refs;return true;}return false;}
void page_free(uintptr_t page)
{for(unsigned i=0;i<512;++i)if(frames[i].address==page&&frames[i].refs){
 if(!--frames[i].refs){free((void *)page);frames[i].address=0;--live;}return;
 }assert(!"invalid/double page release");}
int main(void)
{
 struct shm_backing a={0};assert(!shm_resize(&a,262144)&&live==64);
 unsigned char bytes[32];memset(bytes,0xff,sizeof(bytes));
 assert(shm_read(&a,4090,bytes,sizeof(bytes))==32);
 for(unsigned i=0;i<32;++i)assert(bytes[i]==0);
 assert(shm_write(&a,4090,"shared across pages",19)==19);
 uintptr_t alias=shm_acquire_page(&a,4096);assert(alias&&page_references(alias)==2);
 assert(!shm_acquire_page(&a,4097)&&!shm_acquire_page(&a,262144));
 ((unsigned char *)alias)[0]='X';assert(shm_read(&a,4096,bytes,1)==1&&bytes[0]=='X');
 assert(shm_write(&a,4097,"Y",1)==1&&((char *)alias)[1]=='Y');
 assert(shm_resize(&a,4)==-16&&a.size==262144&&live==64);
 allocations_before_failure=1;assert(shm_resize(&a,300000)==-12);
 assert(a.size==262144&&live==64&&((char *)alias)[1]=='Y'&&a.pages[64]==0);
 allocations_before_failure=-1;assert(!shm_resize(&a,300000));
 assert(shm_read(&a,262144,bytes,32)==32);for(unsigned i=0;i<32;++i)assert(!bytes[i]);
 unsigned before=live;shm_release(&a);assert(live==1&&before>1&&page_references(alias)==1);
 assert(((char *)alias)[0]=='X'&&((char *)alias)[1]=='Y');page_free(alias);assert(!live);
 assert(shm_write(&a,5000,"abc",3)==3&&a.size==5003);
 assert(shm_read(&a,4990,bytes,13)==13);for(unsigned i=0;i<10;++i)assert(!bytes[i]);
 assert(!memcmp(bytes+10,"abc",3));assert(!shm_resize(&a,3)&&live==1);
 assert(!shm_resize(&a,5003));assert(shm_read(&a,4990,bytes,13)==13);
 for(unsigned i=0;i<13;++i)assert(!bytes[i]);
 assert(shm_resize(&a,SHM_MAX_BYTES+1)==-27&&a.size==5003);
 assert(shm_write(&a,SHM_MAX_BYTES,"x",1)==-28);
 assert(shm_write(&a,0,0,1)==-14&&shm_read(&a,0,0,1)==-14);
 assert(shm_read(&a,SIZE_MAX,bytes,1)==0&&shm_write(&a,SIZE_MAX,bytes,0)==-28);
 shm_release(&a);assert(!live);
 puts("PASS real shared backing: 256KiB, zero-fill, cross-page I/O/coherence, aliases outlive inode, mapped shrink rejection, allocation rollback and exact frames; host boundary mock, NOT kernel mmap/Cobalt acceptance");
 return 0;
}
