#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
void *trial_memcpy(void *,const void *,size_t);
void *trial_memset(void *,int,size_t);
void *trial_memmove(void *,const void *,size_t);
static unsigned char source[2048],actual[2048],expected[2048];
int main(void){
 for(unsigned i=0;i<sizeof(source);++i)source[i]=(unsigned char)(i*37u+11u);
 for(size_t n=0;n<=512;++n)for(unsigned a=0;a<32;++a)for(unsigned b=0;b<32;++b){
  memset(actual,0x97,sizeof(actual));memset(expected,0x97,sizeof(expected));
  memcpy(expected+b,source+a,n);
  assert(trial_memcpy(actual+b,source+a,n)==actual+b);
  assert(!memcmp(actual,expected,sizeof(actual)));
 }
 const int values[]={-1,0,1,127,255,256};
 for(size_t n=0;n<=512;++n)for(unsigned a=0;a<32;++a)for(unsigned v=0;v<6;++v){
  memset(actual,0x97,sizeof(actual));memset(expected,0x97,sizeof(expected));
  memset(expected+a,values[v],n);
  assert(trial_memset(actual+a,values[v],n)==actual+a);
  assert(!memcmp(actual,expected,sizeof(actual)));
 }
 for(size_t n=0;n<=256;++n)for(unsigned a=0;a<32;++a)for(int delta=-48;delta<=48;++delta){
  memcpy(actual,source,sizeof(actual));memcpy(expected,source,sizeof(expected));
  unsigned start=256+a,end=(unsigned)((int)start+delta);
  memmove(expected+end,expected+start,n);
  assert(trial_memmove(actual+end,actual+start,n)==actual+end);
  assert(!memcmp(actual,expected,sizeof(actual)));
 }
 long page=sysconf(_SC_PAGESIZE);assert(page>=4096);
 unsigned char *s=mmap(0,(size_t)page*3,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 unsigned char *d=mmap(0,(size_t)page*3,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 assert(s!=MAP_FAILED&&d!=MAP_FAILED);
 assert(!mprotect(s+page,(size_t)page,PROT_READ|PROT_WRITE));
 assert(!mprotect(d+page,(size_t)page,PROT_READ|PROT_WRITE));
 memset(s+page,0x35,(size_t)page);
 for(size_t n=0;n<=512;++n){
  unsigned char *sp=s+2*page-n,*dp=d+2*page-n;
  assert(trial_memcpy(dp,sp,n)==dp);assert(!memcmp(dp,sp,n));
  assert(trial_memset(dp,0x94,n)==dp);
  for(size_t i=0;i<n;++i)assert(dp[i]==0x94);
 }
 for(size_t n=1;n<=512;++n)for(unsigned distance=1;distance<=31;++distance){
  unsigned char *p=d+2*page-n-distance;
  memcpy(p,source,n+distance);memcpy(expected,source,n+distance);
  memmove(expected+distance,expected,n);
  trial_memmove(p+distance,p,n);assert(!memcmp(p,expected,n+distance));
 }
 assert(!munmap(s,(size_t)page*3));assert(!munmap(d,(size_t)page*3));
 puts("PASS SDK word copy: alignments/tails/whole-buffer guards/overlap/PROT_NONE boundaries; host correctness only, not native performance");
 return 0;
}
