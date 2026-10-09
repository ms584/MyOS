/* Native decoder error, ownership, immediate teardown and rapid-seek checks. */
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t);
#include "starboard/configuration.h"
#include "starboard/player.h"
#include "starboard/log.h"
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static int needs[2],errors,freed,latest;static SbPlayer current;
static void nap(void){struct timespec t={0,10000000};clock_nanosleep(CLOCK_MONOTONIC,0,&t,0);}
static void dealloc(SbPlayer p,void *ctx,const void *buffer){(void)p;(void)ctx;free((void *)buffer);pthread_mutex_lock(&mutex);++freed;pthread_mutex_unlock(&mutex);}
static void decoder(SbPlayer p,void *ctx,SbMediaType type,SbPlayerDecoderState state,int ticket){
 (void)p;(void)ctx;pthread_mutex_lock(&mutex);
 if(ticket==latest&&state==kSbPlayerDecoderStateNeedsData)needs[type]=1;
 pthread_mutex_unlock(&mutex);
}
static void status(SbPlayer p,void *ctx,SbPlayerState state,int ticket){(void)p;(void)ctx;(void)state;(void)ticket;}
static void error(SbPlayer p,void *ctx,SbPlayerError code,const char *message){
 (void)p;(void)ctx;(void)message;if(code!=kSbPlayerErrorDecode)return;
 pthread_mutex_lock(&mutex);++errors;pthread_mutex_unlock(&mutex);
}
static int wait_value(int *value){
 for(int i=0;i<1000;++i){pthread_mutex_lock(&mutex);int ready=*value;pthread_mutex_unlock(&mutex);if(ready)return 0;nap();}
 return -1;
}
static void *run(void *unused){
 (void)unused;
 for(int type=0;type<2;++type){
  SbPlayerCreationParam param={0};param.output_mode=kSbPlayerOutputModeDecodeToTexture;
  if(type==0){param.audio_stream_info.codec=kSbMediaAudioCodecOpus;param.audio_stream_info.number_of_channels=1;param.audio_stream_info.samples_per_second=48000;}
  else{param.video_stream_info.codec=kSbMediaVideoCodecVp8;param.video_stream_info.frame_width=160;param.video_stream_info.frame_height=90;}
  /* Repeat creation/destruction before any seek, without device/worker leak. */
  for(int cycle=0;cycle<3;++cycle){
   current=SbPlayerCreate(0,&param,dealloc,decoder,status,error,0,0);if(!current)return (void *)1;
   SbPlayerDestroy(current);
  }
  pthread_mutex_lock(&mutex);latest=43;needs[0]=needs[1]=errors=freed=0;pthread_mutex_unlock(&mutex);
  current=SbPlayerCreate(0,&param,dealloc,decoder,status,error,0,0);if(!current)return (void *)2;
  SbPlayerSeek(current,0,41);SbPlayerSeek(current,1000000,42);SbPlayerSeek(current,0,43);
  if(wait_value(&needs[type])){SbPlayerDestroy(current);return (void *)3;}
  int count=SbPlayerGetMaximumNumberOfSamplesPerWrite(current,(SbMediaType)type);
  if(count<1||count>8){SbPlayerDestroy(current);return (void *)4;}
  SbPlayerSampleInfo samples[8]={{0}};
  for(int i=0;i<count;++i){
   unsigned char *buffer=malloc(1);
   if(!buffer){for(int j=0;j<i;++j)free((void *)samples[j].buffer);SbPlayerDestroy(current);return (void *)4;}
   buffer[0]=type==0?0xff:0;
   samples[i].type=(SbMediaType)type;samples[i].buffer=buffer;samples[i].buffer_size=1;
  }
  SbPlayerWriteSamples(current,(SbMediaType)type,samples,count);
  if(wait_value(&errors)){SbPlayerDestroy(current);return (void *)5;}
  SbPlayerDestroy(current);if(freed!=count||errors!=1)return (void *)6;
  SbLogRaw(type==0?"native Opus corrupt/error/sample ownership/rapid seek/immediate teardown PASS\n":"native VP8 corrupt/error/sample ownership/rapid seek/immediate teardown PASS\n");
 }
 /* Exhaust real guest pages, then verify Create fails closed and recovers
  * after releasing the pressure. No host-memory injection or fake allocator. */
 void *held[64];unsigned count=0;
 while(count<64){
  void *block=mmap(0,1024u*1024u,3,0x22,-1,0);
  if(block==(void *)(uintptr_t)-1)break;held[count++]=block;
 }
 if(!count)return (void *)7;
 SbPlayerCreationParam param={0};param.output_mode=kSbPlayerOutputModeDecodeToTexture;
 param.audio_stream_info.codec=kSbMediaAudioCodecOpus;
 param.audio_stream_info.number_of_channels=1;param.audio_stream_info.samples_per_second=48000;
 current=SbPlayerCreate(0,&param,dealloc,decoder,status,error,0,0);
 int rejected=current==0;if(current)SbPlayerDestroy(current);
 for(unsigned i=0;i<count;++i)if(munmap(held[i],1024u*1024u))return (void *)8;
 if(!rejected)return (void *)9;
 current=SbPlayerCreate(0,&param,dealloc,decoder,status,error,0,0);
 if(!current)return (void *)12;SbPlayerDestroy(current);
 SbLogRaw("native media guest page exhaustion/recovery PASS\n");
 SbLogRaw("native media focused lifecycle faults PASS; NOT full fault acceptance\n");
 return (void *)96;
}
int _start(void){
 pthread_attr_t a;pthread_t t;void *result=0;
 if(pthread_attr_init(&a)||pthread_attr_setstacksize(&a,2u*1024u*1024u))return 10;
 int rc=pthread_create(&t,&a,run,0);pthread_attr_destroy(&a);
 if(rc||pthread_join(t,&result))return 11;return (int)(uintptr_t)result;
}
