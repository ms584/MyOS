#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "pcm-call.h"
extern void SbLogRaw(const char *);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int mprotect(void *,size_t,int),munmap(void *,size_t);
static int64_t invoke(struct pcm_request *r){
 register uint64_t a __asm__("rax")=51;
 __asm__ volatile("int $0x80":"+a"(a):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)r),"d"((uint64_t)0):"memory","cc");return (int64_t)a;
}
int _start(void){
 struct pcm_status status={0};struct pcm_request request={PCM_QUERY,0,0,0,(uintptr_t)&status};
 if(invoke(&request))return 1;
#if !EXPECT_AUDIO
 if(status.capacity||status.channels||status.sample_rate||status.sample_bits)return 2;
 request.action=PCM_ACQUIRE;if(invoke(&request)!=-19)return 3;
 SbLogRaw("native PCM missing-device fail-closed PASS\n");return 91;
#else
 if(status.channels!=1||status.sample_rate!=11025||status.sample_bits!=8||status.capacity!=65536)return 4;
 unsigned char *pages=mmap(0,8192,3,0x22,-1,0);if(pages==(void *)(uintptr_t)-1)return 5;
 for(unsigned i=0;i<16;++i)pages[4096-16+i]='G';
 if(mprotect(pages+4096,4096,1))return 6;
 request.action=PCM_ACQUIRE;request.result=(uintptr_t)(pages+4096-16);
 if(invoke(&request)!=-14)return 7;
 for(unsigned i=0;i<16;++i)if(pages[4096-16+i]!='G')return 8;
 request.result=(uintptr_t)&status;if(invoke(&request))return 9;
 struct pcm_status canary;for(unsigned i=0;i<sizeof(canary);++i)((unsigned char *)&canary)[i]='G';
 request.result=(uintptr_t)&canary;if(invoke(&request)!=-16)return 10;
 for(unsigned i=0;i<sizeof(canary);++i)if(((unsigned char *)&canary)[i]!='G')return 11;
 unsigned char samples[4096];unsigned phase=0;
 request=(struct pcm_request){PCM_QUEUE,0,(uintptr_t)samples,4096,0};
 for(unsigned block=0;block<4;++block){
  for(unsigned i=0;i<4096;++i){phase+=440;if(phase>=11025)phase-=11025;samples[i]=phase<5512?200:56;}
  if(invoke(&request))return 12;
 }
 request=(struct pcm_request){PCM_STATUS,0,0,0,(uintptr_t)&status};
 struct timespec sleep={0,10000000};unsigned waits=0;
 do{if(invoke(&request)||status.consumed>16384)return 13;
  if(status.consumed==16384)break;
  if(clock_nanosleep(1,0,&sleep,0))return 14;
 }while(++waits<1000);
 if(status.consumed!=16384||status.queued||status.dma_frames)return 15;
 request=(struct pcm_request){PCM_QUEUE,0,(uintptr_t)samples,4096,0};if(invoke(&request))return 16;
 request=(struct pcm_request){PCM_FLUSH,0,0,0,0};if(invoke(&request))return 17;
 request=(struct pcm_request){PCM_STATUS,0,0,0,(uintptr_t)&status};if(invoke(&request)||status.consumed!=16384||status.queued||status.dma_frames)return 18;
 /* A short EOS starts explicitly rather than waiting forever for prebuffer. */
 request=(struct pcm_request){PCM_QUEUE,0,(uintptr_t)samples,4096,0};
 if(invoke(&request)||invoke(&request))return 21;
 request=(struct pcm_request){PCM_STATUS,0,0,0,(uintptr_t)&status};
 if(invoke(&request)||status.consumed!=16384||status.queued!=8192||status.dma_frames)return 22;
 request=(struct pcm_request){PCM_START,0,0,0,0};if(invoke(&request))return 23;
 request=(struct pcm_request){PCM_STATUS,0,0,0,(uintptr_t)&status};waits=0;
 do{if(invoke(&request)||status.consumed>24576)return 24;
  if(status.consumed>16384)break;
  if(clock_nanosleep(1,0,&sleep,0))return 25;
 }while(++waits<100);
 if(status.consumed<=16384||status.consumed>=24576)return 26;
 uint64_t before_pause=status.consumed;
 request=(struct pcm_request){PCM_FLUSH,0,0,0,0};if(invoke(&request))return 27;
 request=(struct pcm_request){PCM_STATUS,0,0,0,(uintptr_t)&status};
 if(invoke(&request)||status.consumed<before_pause||status.consumed>24576||status.queued||status.dma_frames)return 28;
 uint64_t paused=status.consumed;
 if(clock_nanosleep(1,0,&sleep,0)||invoke(&request)||status.consumed!=paused)return 29;
 if(munmap(pages,8192))return 19;
#if LEAK_AUDIO
 SbLogRaw("native PCM owned IRQ/flush/pointer PASS; automatic process cleanup required\n");
#else
 request=(struct pcm_request){PCM_RELEASE,0,0,0,0};if(invoke(&request)||invoke(&request)!=-9)return 20;
 SbLogRaw("native PCM owned IRQ/flush/pointer/release PASS\n");
#endif
 return 92;
#endif
}
