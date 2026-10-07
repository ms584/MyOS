#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "audio-sink-abi.h"
#include "thread-limit.h"
#ifndef TERMINATION_MODE
#define TERMINATION_MODE 0
#endif
extern void SbLogRaw(const char *);
struct context {
 pthread_t main;SbAudioSink sink;
 volatile unsigned consumed,updates,playing,block,entered,allow,finished,error,malformed;
};
static void sleep_ms(unsigned ms){struct timespec t={ms/1000,(ms%1000)*1000000};(void)clock_nanosleep(CLOCK_MONOTONIC,0,&t,0);}
static void update(int *frames,int *offset,bool *playing,bool *eos,void *raw){
 struct context *c=raw;if(pthread_equal(pthread_self(),c->main))__atomic_store_n(&c->error,1,__ATOMIC_RELEASE);
 if(SbAudioSinkIsValid(0)||SbAudioSinkGetMaxChannels()!=1)__atomic_store_n(&c->error,2,__ATOMIC_RELEASE);
 __atomic_add_fetch(&c->updates,1,__ATOMIC_RELAXED);
 if(__atomic_load_n(&c->block,__ATOMIC_ACQUIRE)){
  __atomic_store_n(&c->entered,1,__ATOMIC_RELEASE);
  while(!__atomic_load_n(&c->allow,__ATOMIC_ACQUIRE))sleep_ms(10);
 }
 unsigned consumed=__atomic_load_n(&c->consumed,__ATOMIC_ACQUIRE);
 *frames=8192-(int)consumed;*offset=(14336+(int)consumed)%16384;
 *playing=__atomic_load_n(&c->playing,__ATOMIC_ACQUIRE)!=0;*eos=true;
 if(c->malformed==1)*frames=-1;
 if(c->malformed==2)*frames=16385;
 if(c->malformed==3)*offset=-1;
 if(c->malformed==4)*offset=16384;
}
static void consume(int count,void *raw){
 struct context *c=raw;if(pthread_equal(pthread_self(),c->main)||count<=0)__atomic_store_n(&c->error,3,__ATOMIC_RELEASE);
 unsigned total=__atomic_add_fetch(&c->consumed,(unsigned)count,__ATOMIC_ACQ_REL);
 if(total>8192)__atomic_store_n(&c->error,4,__ATOMIC_RELEASE);
}
#if EXPECT_AUDIO
static void *destroy(void *raw){struct context *c=raw;SbAudioSinkDestroy(c->sink);__atomic_store_n(&c->finished,1,__ATOMIC_RELEASE);return 0;}
static void *occupy(void *raw){volatile unsigned *stop=raw;while(!__atomic_load_n(stop,__ATOMIC_ACQUIRE))sleep_ms(10);return 0;}
#endif
int _start(void){
 if(!SbAudioSinkIsAudioSampleTypeSupported(kSbMediaAudioSampleTypeFloat32)||
    !SbAudioSinkIsAudioSampleTypeSupported(kSbMediaAudioSampleTypeInt16Deprecated)||
    SbAudioSinkIsAudioSampleTypeSupported((SbMediaAudioSampleType)99))return 1;
 if(SbAudioSinkGetNearestSupportedSampleFrequency(48000)!=11025||
    SbAudioSinkGetMinBufferSizeInFrames(1,kSbMediaAudioSampleTypeFloat32,11025)!=16384||
    SbAudioSinkGetMinBufferSizeInFrames(2,kSbMediaAudioSampleTypeFloat32,11025)!=-1||
    SbAudioSinkGetMinBufferSizeInFrames(1,kSbMediaAudioSampleTypeFloat32,44100)!=-1)return 2;
 struct context c={0};c.main=pthread_self();c.playing=1;
 void *buffer=malloc(16384*4);if(!buffer)return 3;void *buffers[]={buffer};unsigned phase=0;
 for(unsigned i=0;i<16384;++i){phase+=440;if(phase>=11025)phase-=11025;
#if SAMPLE_FLOAT
  ((float *)buffer)[i]=phase<5512?0.5625f:-0.5625f;
#else
  ((int16_t *)buffer)[i]=phase<5512?18432:-18432;
#endif
 }
 SbMediaAudioSampleType type=SAMPLE_FLOAT?kSbMediaAudioSampleTypeFloat32:kSbMediaAudioSampleTypeInt16Deprecated;
#if EXPECT_AUDIO
 volatile unsigned stop=0;pthread_t slots[MYOS_WORKER_LIMIT];
 pthread_attr_t occupancy_attr;
 if(pthread_attr_init(&occupancy_attr)||pthread_attr_setstacksize(&occupancy_attr,65536))return 16;
 for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)
  if(pthread_create(&slots[i],&occupancy_attr,occupy,(void *)&stop))return 16;
 if(pthread_attr_destroy(&occupancy_attr))return 16;
 if(SbAudioSinkCreate(1,11025,type,kSbMediaAudioFrameStorageTypeInterleaved,buffers,16384,update,consume,&c)||c.updates)return 17;
 __atomic_store_n(&stop,1,__ATOMIC_RELEASE);
 for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)if(pthread_join(slots[i],0))return 18;
 for(unsigned invalid=1;invalid<=4;++invalid){
  c.malformed=invalid;c.updates=0;
  if(SbAudioSinkCreate(1,11025,type,kSbMediaAudioFrameStorageTypeInterleaved,buffers,16384,update,consume,&c)||!c.updates||c.consumed||c.error)return 19;
 }
 c.malformed=0;c.updates=0;
#endif
 if(SbAudioSinkCreate(2,11025,type,kSbMediaAudioFrameStorageTypeInterleaved,buffers,16384,update,consume,&c)||c.updates)return 4;
 c.sink=SbAudioSinkCreate(1,11025,type,kSbMediaAudioFrameStorageTypeInterleaved,buffers,16384,update,consume,&c);
#if !EXPECT_AUDIO
 if(c.sink||c.updates||c.consumed)return 5;
 free(buffer);
 SbLogRaw("native typed sink absent-device/no callback PASS\n");return 93;
#else
 if(!c.sink||!c.updates||!SbAudioSinkIsValid(c.sink)||c.error)return 6;
 if(TERMINATION_MODE){
  sleep_ms(80);
  SbLogRaw("native typed sink active worker/device lease termination ready\n");
  if(TERMINATION_MODE==1)__asm__ volatile("ud2");
  for(;;)sleep_ms(10);
 }
 if(SbAudioSinkCreate(1,11025,type,kSbMediaAudioFrameStorageTypePlanar,buffers,16384,update,consume,&c))return 7;
 unsigned waits=0;
 while(__atomic_load_n(&c.consumed,__ATOMIC_ACQUIRE)<1000&&waits++<200)sleep_ms(10);
 if(c.consumed<1000||c.consumed>=8192||c.error)return 8;
 __atomic_store_n(&c.playing,0,__ATOMIC_RELEASE);sleep_ms(50);
 unsigned paused=__atomic_load_n(&c.consumed,__ATOMIC_ACQUIRE);sleep_ms(40);
 if(paused!=__atomic_load_n(&c.consumed,__ATOMIC_ACQUIRE)||paused>=8192)return 9;
 __atomic_store_n(&c.playing,1,__ATOMIC_RELEASE);waits=0;
 while(__atomic_load_n(&c.consumed,__ATOMIC_ACQUIRE)!=8192&&waits++<300)sleep_ms(10);
 if(c.consumed!=8192||c.error)return 10;
 __atomic_store_n(&c.block,1,__ATOMIC_RELEASE);waits=0;
 while(!__atomic_load_n(&c.entered,__ATOMIC_ACQUIRE)&&waits++<100)sleep_ms(10);
 if(!c.entered)return 11;
 pthread_t thread;if(pthread_create(&thread,0,destroy,&c))return 12;
 sleep_ms(50);if(__atomic_load_n(&c.finished,__ATOMIC_ACQUIRE))return 13;
 __atomic_store_n(&c.allow,1,__ATOMIC_RELEASE);if(pthread_join(thread,0)||!c.finished)return 14;
 unsigned updates=__atomic_load_n(&c.updates,__ATOMIC_ACQUIRE);sleep_ms(30);
 if(updates!=c.updates||c.error||SbAudioSinkIsValid(c.sink))return 15;
 free(buffer);
 SbLogRaw("native typed sink Float32/Int16/initial internal callback/mono/EOS/circular/pause/Destroy waits/no late callback/thread exhaustion/malformed source rollback PASS\n");return 94;
#endif
}
