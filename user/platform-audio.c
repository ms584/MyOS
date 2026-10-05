/* PRIVATE Starboard PCM sink. No codec/player capability or dummy device. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "audio-sink-abi.h"
#include "pcm-call.h"
struct SbAudioSinkPrivate {
 pthread_t worker;
 pthread_mutex_t mutex;
 pthread_cond_t condition;
 int ready;
 volatile unsigned stop;
 unsigned destroying;
 SbMediaAudioSampleType type;
 SbAudioSinkFrameBuffers buffers;
 int capacity;
 SbAudioSinkUpdateSourceStatusFunc update;
 SbAudioSinkConsumeFramesFunc consume;
 void *context;
 uint64_t submitted,acknowledged;
};
struct audio_registry {pthread_mutex_t mutex;SbAudioSink sink;};
/* Shared main RW/NX TLS bytes1888..1975, after allocator1744..1887. */
static struct audio_registry *registry(void){return (struct audio_registry *)(uintptr_t)0x803f010760ull;}
_Static_assert(sizeof(struct audio_registry)==88,"audio shared TLS slot");
static int64_t pcm(struct pcm_request *r){
 register uint64_t a __asm__("rax")=51;
 __asm__ volatile("int $0x80":"+a"(a):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)r),"d"((uint64_t)0):"memory","cc");return (int64_t)a;
}
static int operation(unsigned action){struct pcm_request r={action,0,0,0,0};return (int)pcm(&r);}
static int status(struct pcm_status *s){struct pcm_request r={PCM_STATUS,0,0,0,(uintptr_t)s};return (int)pcm(&r);}
static int acknowledge(SbAudioSink sink,const struct pcm_status *s){
 if(s->consumed<sink->acknowledged||s->consumed>sink->submitted)return -1;
 unsigned count=(unsigned)(s->consumed-sink->acknowledged);
 if(count){sink->consume((int)count,sink->context);sink->acknowledged=s->consumed;}
 return 0;
}
static uint8_t sample(SbAudioSink sink,unsigned index){
 const unsigned char *data=sink->buffers[0];
 if(sink->type==kSbMediaAudioSampleTypeInt16Deprecated){
  uint16_t raw=(uint16_t)data[index*2]|((uint16_t)data[index*2+1]<<8);
  return (uint8_t)(((int)(int16_t)raw>>8)+128);
 }
 union {float value;unsigned char bytes[4];} input;
 for(unsigned i=0;i<4;++i)input.bytes[i]=data[index*4+i];
 float value=input.value;if(value!=value)return 128;
 if(value>=1.0f)return 255;if(value<=-1.0f)return 0;
 return (uint8_t)((int)(value*128.0f)+128);
}
static int source(SbAudioSink sink,int *frames,int *offset,bool *playing,bool *eos){
 *frames=*offset=0;*playing=*eos=false;
 sink->update(frames,offset,playing,eos,sink->context);
 return *frames<0||*frames>sink->capacity||*offset<0||*offset>=sink->capacity?-1:0;
}
static void publish_ready(SbAudioSink sink,int ready){
 pthread_mutex_lock(&sink->mutex);sink->ready=ready;
 pthread_cond_broadcast(&sink->condition);pthread_mutex_unlock(&sink->mutex);
}
static void *audio_worker(void *context){
 SbAudioSink sink=context;int frames,offset;bool playing,eos;
 /* Required initial update callback on this internal thread, before Create returns. */
 if(source(sink,&frames,&offset,&playing,&eos)){publish_ready(sink,-1);return 0;}
 publish_ready(sink,1);
 struct timespec interval={0,10000000};
 while(!__atomic_load_n(&sink->stop,__ATOMIC_ACQUIRE)){
  struct pcm_status hardware;if(status(&hardware)||acknowledge(sink,&hardware))break;
  if(source(sink,&frames,&offset,&playing,&eos))break;
  if(!playing){
   if(operation(PCM_FLUSH)||status(&hardware)||acknowledge(sink,&hardware))break;
   sink->submitted=sink->acknowledged;
  }else{
   uint64_t pending=sink->submitted-sink->acknowledged;
   if(pending>(unsigned)frames)break;
   unsigned available=(unsigned)frames-(unsigned)pending;
   unsigned room=hardware.capacity-hardware.queued;
   if(available>room)available=room;
   while(available&&!__atomic_load_n(&sink->stop,__ATOMIC_ACQUIRE)){
    unsigned chunk=available>4096?4096:available;
    unsigned char pcm_bytes[4096];
    for(unsigned i=0;i<chunk;++i)pcm_bytes[i]=sample(sink,((unsigned)offset+(unsigned)pending+i)%(unsigned)sink->capacity);
    struct pcm_request request={PCM_QUEUE,0,(uintptr_t)pcm_bytes,chunk,0};
    if(pcm(&request))goto finished;
    sink->submitted+=chunk;pending+=chunk;available-=chunk;
   }
   /* Short EOS must start now, even below the ordinary16384-frame prebuffer. */
   if(eos&&sink->submitted>sink->acknowledged&&operation(PCM_START))break;
  }
  (void)clock_nanosleep(CLOCK_MONOTONIC,0,&interval,0);
 }
 /* Device lease is released by Destroy or process-stop, not by a stale worker
  * that could race a subsequent owner. No callback after this worker returns. */
finished:
 return 0;
}
bool SbAudioSinkIsValid(SbAudioSink sink){
 struct audio_registry *r=registry();pthread_mutex_lock(&r->mutex);
 bool valid=sink&&r->sink==sink&&!sink->destroying;pthread_mutex_unlock(&r->mutex);return valid;
}
int SbAudioSinkGetMaxChannels(void){return 1;}
int SbAudioSinkGetNearestSupportedSampleFrequency(int frequency){(void)frequency;return 11025;}
bool SbAudioSinkIsAudioSampleTypeSupported(SbMediaAudioSampleType type){return type==kSbMediaAudioSampleTypeInt16Deprecated||type==kSbMediaAudioSampleTypeFloat32;}
bool SbAudioSinkIsAudioFrameStorageTypeSupported(SbMediaAudioFrameStorageType type){return type==kSbMediaAudioFrameStorageTypeInterleaved||type==kSbMediaAudioFrameStorageTypePlanar;}
int SbAudioSinkGetMinBufferSizeInFrames(int channels,SbMediaAudioSampleType type,int frequency){
 return channels==1&&frequency==11025&&SbAudioSinkIsAudioSampleTypeSupported(type)?16384:-1;
}
SbAudioSink SbAudioSinkCreate(int channels,int frequency,SbMediaAudioSampleType type,
 SbMediaAudioFrameStorageType storage,SbAudioSinkFrameBuffers buffers,int capacity,
 SbAudioSinkUpdateSourceStatusFunc update,SbAudioSinkConsumeFramesFunc consume,void *context){
 if(SbAudioSinkGetMinBufferSizeInFrames(channels,type,frequency)<0||
    !SbAudioSinkIsAudioFrameStorageTypeSupported(storage)||!buffers||!buffers[0]||
    capacity<16384||capacity>1048576||!update||!consume)return 0;
 struct audio_registry *r=registry();pthread_mutex_lock(&r->mutex);
 if(r->sink){pthread_mutex_unlock(&r->mutex);return 0;}
 struct pcm_status hardware;struct pcm_request request={PCM_ACQUIRE,0,0,0,(uintptr_t)&hardware};
 if(pcm(&request)){pthread_mutex_unlock(&r->mutex);return 0;}
 SbAudioSink sink=calloc(1,sizeof(*sink));
 if(!sink){operation(PCM_RELEASE);pthread_mutex_unlock(&r->mutex);return 0;}
 sink->type=type;sink->buffers=buffers;sink->capacity=capacity;
 sink->update=update;sink->consume=consume;sink->context=context;
 if(pthread_mutex_init(&sink->mutex,0)){
  free(sink);operation(PCM_RELEASE);pthread_mutex_unlock(&r->mutex);return 0;
 }
 if(pthread_cond_init(&sink->condition,0)){
  pthread_mutex_destroy(&sink->mutex);free(sink);operation(PCM_RELEASE);
  pthread_mutex_unlock(&r->mutex);return 0;
 }
 r->sink=sink;pthread_mutex_unlock(&r->mutex);
 if(pthread_create(&sink->worker,0,audio_worker,sink)){
  pthread_cond_destroy(&sink->condition);pthread_mutex_destroy(&sink->mutex);
  operation(PCM_RELEASE);pthread_mutex_lock(&r->mutex);r->sink=0;
  pthread_mutex_unlock(&r->mutex);free(sink);return 0;
 }
 /* Never hold registry lock across application callbacks, including initial
  * callback; reserve the slot before releasing lock and waiting for readiness. */
 pthread_mutex_lock(&sink->mutex);
 while(!sink->ready)pthread_cond_wait(&sink->condition,&sink->mutex);
 int ready=sink->ready;pthread_mutex_unlock(&sink->mutex);
 if(ready<0){SbAudioSinkDestroy(sink);return 0;}return sink;
}
void SbAudioSinkDestroy(SbAudioSink sink){
 struct audio_registry *r=registry();pthread_mutex_lock(&r->mutex);
 if(!sink||r->sink!=sink||sink->destroying||pthread_equal(pthread_self(),sink->worker)){
  pthread_mutex_unlock(&r->mutex);return;
 }
 sink->destroying=1;__atomic_store_n(&sink->stop,1,__ATOMIC_RELEASE);
 pthread_mutex_unlock(&r->mutex);
 if(pthread_join(sink->worker,0))return; /* never free buffers under active callback */
 operation(PCM_RELEASE);
 pthread_cond_destroy(&sink->condition);pthread_mutex_destroy(&sink->mutex);
 pthread_mutex_lock(&r->mutex);r->sink=0;pthread_mutex_unlock(&r->mutex);free(sink);
}
