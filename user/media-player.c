/* Native bounded VP8/Opus Starboard18 provider. No host media services. */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#if defined(MYOS_MEDIA_TRACE) || defined(MYOS_MEDIA_NATIVE_UNDERLAY)
#include <stdio.h>
#include "starboard/log.h"
#ifdef MYOS_MEDIA_TRACE
#include "starboard/system.h"
#endif
#endif
#ifdef MYOS_MEDIA_TRACE
#define MEDIA_TRACE(s) SbLogRaw(s)
#else
#define MEDIA_TRACE(s) ((void)0)
#endif
#include "starboard/player.h"
#include "starboard/gles.h"
#include "media-player-audio.h"
#include "media-provider-abi.h"
#include "pcm-call.h"
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
#include "media-video-output.h"
#endif
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "opus.h"
#define MAX_W 160
#define MAX_H 90
#ifdef MYOS_MEDIA_VP9
#define DECODE_W 640
#define DECODE_H 360
#else
#define DECODE_W MAX_W
#define DECODE_H MAX_H
#endif
#define PIXELS (MAX_W*MAX_H*4)
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
#define VIDEO_SLOTS 16
#else
#define VIDEO_SLOTS 4
#endif
#define PCM_CAP 32768
#define PCM_RATE 11025
#define MAX_PACKET (256*1024)
#define AUDIO_BATCH 8
#define VIDEO_BATCH 4
#define TARGET_LIMIT 8
const uint64_t MyOSMediaProviderAbi=MYOS_MEDIA_PROVIDER_VERSION;
struct frame {int width,height;int64_t time;unsigned char rgba[PIXELS];};
struct pending {SbPlayerSampleInfo samples[AUDIO_BATCH];unsigned read;int full,requested,eos;};
struct SbPlayerPrivate {
 pthread_mutex_t mutex;pthread_t worker;unsigned published,closing,stop;
 SbPlayerDeallocateSampleFunc deallocate;SbPlayerDecoderStatusFunc decoder;
 SbPlayerStatusFunc status;SbPlayerErrorFunc error;void *context;
 unsigned seek_revision;int seeked;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 int native_video,bounds_x,bounds_y,bounds_width,bounds_height,video_shown;
 uint64_t video_window;unsigned bounds_revision,shown_bounds,shown_generation;
 unsigned native_presented,native_sync_samples;int64_t native_max_sync;
#ifdef MYOS_MEDIA_VP9
 unsigned decoded_format_logged;
 unsigned native_hardware_clock_reads,native_hardware_clock_failures,native_hardware_ack_lag_frames;
 int native_rebuffering;unsigned native_rebuffers;int64_t native_presentation_max_us;
#endif
 int native_first_video_pending;
#ifdef MYOS_MEDIA_TRACE
 int64_t diagnostic_decode_max_us,diagnostic_present_max_us,diagnostic_wake_max_us;
#endif
#endif
#ifdef MYOS_MEDIA_TRACE
 unsigned trace_ticks;
#endif
 int has_audio,has_video,channels,ticket,seek_pending,presenting,ended,failed,error_pending;
 #ifdef MYOS_MEDIA_VP9
 SbMediaVideoCodec video_codec;
#endif
 int video_initialized;vpx_codec_ctx_t video;OpusDecoder *audio;SbAudioSink sink;void *sink_buffers[1];
 struct pending pending[2];struct frame frames[VIDEO_SLOTS],current;
 unsigned video_read,video_count,have_current,generation,rendered_generation;
 int16_t pcm[PCM_CAP];unsigned pcm_read,pcm_count,resample_phase;
 uint64_t consumed;int64_t seek_time,audio_anchor,last_audio_pts,last_video_pts;
 int audio_anchor_set;double rate,volume;
 int64_t wall_anchor,wall_media,duration;
 unsigned total_video,dropped_video,corrupted_video,source_starvations,source_starved;
};
struct SbDecodeTargetPrivate {
 struct SbDecodeTargetPrivate *next;SbDecodeTargetInfo info;
};
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static SbPlayer active;
static pthread_mutex_t targets_mutex=PTHREAD_MUTEX_INITIALIZER;
static SbDecodeTarget targets;static unsigned target_count;
static int64_t now_us(void){struct timespec ts;if(clock_gettime(CLOCK_MONOTONIC,&ts))return 0;return (int64_t)ts.tv_sec*1000000+ts.tv_nsec/1000;}
static SbPlayer lock_player(SbPlayer p){
 pthread_mutex_lock(&registry);
 if(!p||p!=active||__atomic_load_n(&p->closing,__ATOMIC_ACQUIRE)){pthread_mutex_unlock(&registry);return 0;}
 pthread_mutex_lock(&p->mutex);pthread_mutex_unlock(&registry);return p;
}
static void unlock_player(SbPlayer p){pthread_mutex_unlock(&p->mutex);}
static int64_t clock_locked(SbPlayer p){
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 if(p->has_audio){
  uint64_t played=p->consumed;
  /* Presentation holds this mutex, so the sink's consume callback cannot
   * acknowledge DMA during it. Read the real device counter for clocking;
   * only the sink callback owns PCM-ring acknowledgement and reclamation. */
  if(p->sink&&p->audio_anchor_set&&!p->seek_pending){
   struct pcm_status hardware;struct pcm_request request={PCM_STATUS,0,0,0,(uintptr_t)&hardware};
   register uint64_t result __asm__("rax")=51;
   __asm__ volatile("int $0x80":"+a"(result):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)&request),"d"((uint64_t)0):"memory","cc");
   ++p->native_hardware_clock_reads;
   if((int64_t)result||hardware.consumed<p->consumed||hardware.consumed-p->consumed>p->pcm_count){
    ++p->native_hardware_clock_failures;p->error_pending=1;
   }else {
    unsigned lag=(unsigned)(hardware.consumed-p->consumed);
    if(lag>p->native_hardware_ack_lag_frames)p->native_hardware_ack_lag_frames=lag;
    played=hardware.consumed;
   }
  }
  return p->audio_anchor_set?p->audio_anchor+(int64_t)(played*1000000/PCM_RATE):p->seek_time;
 }
#else
 if(p->has_audio)return p->audio_anchor_set?p->audio_anchor+(int64_t)(p->consumed*1000000/PCM_RATE):p->seek_time;
#endif
 return p->wall_media+(p->presenting&&p->rate==1.0?now_us()-p->wall_anchor:0);
}
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
static int64_t native_frame_due_locked(SbPlayer p){
 int64_t lead=10000;
 if(p->native_video&&p->has_audio&&p->presenting&&p->rate==1.0&&!p->native_rebuffering){
  /* Reserve the measured profile's90ms headroom from the first frame,
   * before a new player has presentation history. The real post-display
   * hardware clock and100ms acceptance bound remain unchanged. */
  lead=90000;
 }
 return clock_locked(p)+lead;
}
#endif
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
static int native_pcm_held_locked(SbPlayer p){
 if(!p->sink)return 1;
 struct pcm_status hardware;struct pcm_request request={PCM_STATUS,0,0,0,(uintptr_t)&hardware};
 register uint64_t result __asm__("rax")=51;
 __asm__ volatile("int $0x80":"+a"(result):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)&request),"d"((uint64_t)0):"memory","cc");
 ++p->native_hardware_clock_reads;
 if((int64_t)result){++p->native_hardware_clock_failures;p->error_pending=1;return 0;}
 /* Only the sink owns FLUSH and ring acknowledgement. A rebuffer flag
  * alone cannot prove its async worker has actually stopped queued DMA. */
 return !hardware.queued&&!hardware.dma_frames;
}
#endif
static void source_status(int *frames,int *offset,bool *playing,bool *eos,void *context){
 SbPlayer p=context;pthread_mutex_lock(&p->mutex);
 *frames=(int)p->pcm_count;*offset=(int)p->pcm_read;
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 /* Sink alone owns FLUSH/acknowledgement and replay of unconsumed PCM. */
 if(p->native_video&&p->has_video&&p->presenting&&p->rate==1.0&&
    !p->seek_pending&&!p->stop&&!p->failed&&!p->pending[kSbMediaTypeVideo].eos){
  int64_t clock=clock_locked(p);
  int64_t latest=p->video_count?p->frames[(p->video_read+p->video_count-1)%VIDEO_SLOTS].time:
                  p->have_current?p->current.time:INT64_MIN;
  if(!p->native_rebuffering&&latest<=clock+100000){
   p->native_rebuffering=1;++p->native_rebuffers;
   char detail[192];snprintf(detail,sizeof(detail),
    "MEDIA rebuffer pause clock=%lld latest-video=%lld vq=%u pcm=%u\n",
    (long long)clock,(long long)latest,p->video_count,p->pcm_count);SbLogRaw(detail);
  }
 }
#endif
 *playing=p->presenting&&p->rate==1.0&&!p->seek_pending&&!p->stop&&!p->failed;
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 if(p->native_rebuffering)*playing=false;
#endif
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 /* Presenting state can precede Cobalt's first SetBounds on SPA navigation.
  * Hold PCM until the first punch-out frame was actually submitted. Later
  * hidden/zero bounds do not change normal background-audio behavior. */
 if(p->native_video&&p->has_video&&p->native_first_video_pending)*playing=false;
#endif
 *eos=p->pending[kSbMediaTypeAudio].eos&&!p->pending[kSbMediaTypeAudio].full;
 unsigned starved=*playing&&!*eos&&!*frames;
 if(starved&&!p->source_starved)++p->source_starvations;
 p->source_starved=starved;
 pthread_mutex_unlock(&p->mutex);
}
static void consume_frames(int count,void *context){
 SbPlayer p=context;pthread_mutex_lock(&p->mutex);
 if(count<0||(unsigned)count>p->pcm_count){p->error_pending=1;}
 else {p->pcm_read=(p->pcm_read+(unsigned)count)%PCM_CAP;p->pcm_count-=(unsigned)count;p->consumed+=(unsigned)count;}
 pthread_mutex_unlock(&p->mutex);
}
static int create_sink(SbPlayer p){
 if(!p->has_audio)return 0;
 p->sink_buffers[0]=p->pcm;
 p->sink=SbAudioSinkCreate(1,PCM_RATE,kSbMediaAudioSampleTypeInt16Deprecated,
  kSbMediaAudioFrameStorageTypeInterleaved,p->sink_buffers,PCM_CAP,source_status,consume_frames,p);
 if(!p->sink)return -1;
 /* Opt in only this provider. Existing PCM/sink clients keep the16KiB
  * prebuffer contract; Cobalt's local write horizon needs~200ms startup. */
 struct pcm_request request={PCM_CONFIGURE,0,0,1024,0};
 register uint64_t action __asm__("rax")=51;
 __asm__ volatile("int $0x80":"+a"(action):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)&request),"d"((uint64_t)0):"memory","cc");
 if((int64_t)action){SbAudioSinkDestroy(p->sink);p->sink=0;return -1;}
 return 0;
}
static void release_pending(SbPlayer p){
 const void *buffers[2*AUDIO_BATCH];unsigned count=0;
 pthread_mutex_lock(&p->mutex);
 for(int i=0;i<2;++i){
  struct pending *pending=&p->pending[i];
  while(pending->full){buffers[count++]=pending->samples[pending->read++].buffer;--pending->full;}
  *pending=(struct pending){0};
 }
 pthread_mutex_unlock(&p->mutex);
 for(unsigned i=0;i<count;++i)p->deallocate(p,p->context,buffers[i]);
}
static int reset_decoders(SbPlayer p){
 if(p->video_initialized){vpx_codec_destroy(&p->video);p->video_initialized=0;}
 if(p->has_video){
  vpx_codec_dec_cfg_t cfg={1,DECODE_W,DECODE_H};
  vpx_codec_iface_t *iface=vpx_codec_vp8_dx();
#ifdef MYOS_MEDIA_VP9
  if(p->video_codec==kSbMediaVideoCodecVp9)iface=vpx_codec_vp9_dx();
#endif
  if(vpx_codec_dec_init(&p->video,iface,&cfg,0))return -1;
  p->video_initialized=1;
 }
 if(p->has_audio&&opus_decoder_ctl(p->audio,OPUS_RESET_STATE)!=OPUS_OK)return -1;
 return 0;
}
static unsigned char clamp(int value){return value<0?0:value>255?255:(unsigned char)value;}
static int decode_video_locked(SbPlayer p,const SbPlayerSampleInfo *s){
#if defined(MYOS_MEDIA_TRACE) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 int64_t diagnostic_start=now_us();
#endif
 const unsigned char *data=s->buffer;unsigned bytes=(unsigned)s->buffer_size;
 if(!bytes)return -1;
 #ifdef MYOS_MEDIA_VP9
 if(p->video_codec==kSbMediaVideoCodecVp8)
#endif
 if(!(data[0]&1)){
  if(bytes<10||data[3]!=0x9d||data[4]!=1||data[5]!=0x2a)return -1;
  unsigned w=((unsigned)data[6]|((unsigned)data[7]<<8))&16383;
  unsigned h=((unsigned)data[8]|((unsigned)data[9]<<8))&16383;
  if(!w||!h||w>MAX_W||h>MAX_H)return -1;
 }
 if(s->timestamp<p->last_video_pts)return -1;p->last_video_pts=s->timestamp;
 ++p->total_video;
 if(vpx_codec_decode(&p->video,data,bytes,0,0)){++p->corrupted_video;return -1;}
 vpx_codec_iter_t iter=0;vpx_image_t *image;
 while((image=vpx_codec_get_frame(&p->video,&iter))){
  if(!image->d_w||!image->d_h||image->d_w>DECODE_W||image->d_h>DECODE_H||image->fmt!=VPX_IMG_FMT_I420)return -1;
  if(s->timestamp<p->seek_time){++p->dropped_video;continue;}
  if(p->video_count==VIDEO_SLOTS)return -1;
  struct frame *f=&p->frames[(p->video_read+p->video_count)%VIDEO_SLOTS];
  f->width=(int)image->d_w;f->height=(int)image->d_h;f->time=s->timestamp;
#ifdef MYOS_MEDIA_VP9
  /* Decode full bounded VP9 in MyOS; retain only a160x90 aspect-preserving
   * presentation. This uses the accepted kernel underlay ABI unchanged. */
  if(f->width>MAX_W||f->height>MAX_H){
   if(image->d_w*MAX_H>=image->d_h*MAX_W){
    f->width=MAX_W;f->height=(int)(image->d_h*MAX_W/image->d_w);
   }else{f->height=MAX_H;f->width=(int)(image->d_w*MAX_H/image->d_h);}
   if(f->width<1)f->width=1;if(f->height<1)f->height=1;
  }
#endif
#ifdef MYOS_MEDIA_VP9
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
  if(!p->decoded_format_logged){p->decoded_format_logged=1;char message[160];
   snprintf(message,sizeof(message),"MEDIA decoded codec=%d source=%ux%u presentation=%dx%d\n",
            (int)p->video_codec,image->d_w,image->d_h,f->width,f->height);SbLogRaw(message);}
#endif
  for(unsigned y=0;y<(unsigned)f->height;++y)for(unsigned x=0;x<(unsigned)f->width;++x){
   unsigned sx=x*image->d_w/(unsigned)f->width,sy=y*image->d_h/(unsigned)f->height;
   int Y=image->planes[0][sy*image->stride[0]+sx]-16;
   int U=image->planes[1][(sy/2)*image->stride[1]+sx/2]-128;
   int V=image->planes[2][(sy/2)*image->stride[2]+sx/2]-128;
   unsigned offset=4*(y*(unsigned)f->width+x);
#else
  for(unsigned y=0;y<image->d_h;++y)for(unsigned x=0;x<image->d_w;++x){
   int Y=image->planes[0][y*image->stride[0]+x]-16;
   int U=image->planes[1][(y/2)*image->stride[1]+x/2]-128;
   int V=image->planes[2][(y/2)*image->stride[2]+x/2]-128;
   unsigned offset=4*(y*image->d_w+x);
#endif
   f->rgba[offset]=clamp((298*Y+409*V+128)>>8);
   f->rgba[offset+1]=clamp((298*Y-100*U-208*V+128)>>8);
   f->rgba[offset+2]=clamp((298*Y+516*U+128)>>8);f->rgba[offset+3]=255;
  }
  ++p->video_count;
 }
#if defined(MYOS_MEDIA_TRACE) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 int64_t duration=now_us()-diagnostic_start;
 if(duration>p->diagnostic_decode_max_us)p->diagnostic_decode_max_us=duration;
#endif
 return 0;
}
static int decode_audio_locked(SbPlayer p,const SbPlayerSampleInfo *s){
 if(s->timestamp<p->last_audio_pts)return -1;p->last_audio_pts=s->timestamp;
 int16_t decoded[5760*2];
 int count=opus_decode(p->audio,s->buffer,s->buffer_size,decoded,5760,0);
 if(count<0)return -1;
 int64_t front=s->audio_sample_info.discarded_duration_from_front;
 int64_t back=s->audio_sample_info.discarded_duration_from_back;
 if(front<0||back<0||front>120000||back>120000)return -1;
 int begin=(int)((front*48000+999999)/1000000),end=count-(int)(back*48000/1000000);
 if(begin>end||end>count)return -1;
 for(int i=begin;i<end;++i){
  int64_t stamp=s->timestamp+(int64_t)i*1000000/48000;
  if(stamp<p->seek_time)continue;
  p->resample_phase+=PCM_RATE;
  if(p->resample_phase<48000)continue;p->resample_phase-=48000;
  if(p->pcm_count==PCM_CAP)return -1;
  if(!p->audio_anchor_set){p->audio_anchor=stamp;p->audio_anchor_set=1;}
  int value=p->channels==2?((int)decoded[i*2]+decoded[i*2+1])/2:decoded[i];
  value=(int)(value*p->volume);
  p->pcm[(p->pcm_read+p->pcm_count)%PCM_CAP]=(int16_t)value;++p->pcm_count;
 }
 return 0;
}
static void report_status(SbPlayer p,SbPlayerState state,int ticket){p->status(p,p->context,state,ticket);}
static void *player_worker(void *context){
 SbPlayer p=context;struct timespec interval={0,10000000};
 while(!__atomic_load_n(&p->published,__ATOMIC_ACQUIRE))clock_nanosleep(CLOCK_MONOTONIC,0,&interval,0);
 report_status(p,kSbPlayerStateInitialized,0);
 for(;;){
  pthread_mutex_lock(&p->mutex);
  if(p->stop){pthread_mutex_unlock(&p->mutex);break;}
#ifdef MYOS_MEDIA_TRACE
  if(++p->trace_ticks%100==0){
   char trace[256];snprintf(trace,sizeof(trace),"media trace ticket=%d seek=%d presenting=%d failed=%d pcm=%u consumed=%llu video=%u rate=%d audio-last=%lld video-last=%lld req=%d,%d full=%d,%d\n",
   p->ticket,p->seek_pending,p->presenting,p->failed,p->pcm_count,(unsigned long long)p->consumed,p->video_count,(int)p->rate,
   (long long)p->last_audio_pts,(long long)p->last_video_pts,p->pending[0].requested,p->pending[1].requested,p->pending[0].full,p->pending[1].full);
   SbLogRaw(trace);
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
   char counters[512];snprintf(counters,sizeof(counters),
    "MEDIA metrics mono-us=%lld position-us=%lld presented=%u sync-samples=%u max-sync-us=%lld decoded=%u dropped=%u corrupted=%u starvations=%u consumed=%llu rate=%d ram-query-bytes=%lld decode-max-us=%lld present-max-us=%lld wake-max-us=%lld\n",
    (long long)now_us(),(long long)clock_locked(p),p->native_presented,p->native_sync_samples,
    (long long)p->native_max_sync,p->total_video,p->dropped_video,p->corrupted_video,p->source_starvations,
    (unsigned long long)p->consumed,(int)p->rate,(long long)SbSystemGetUsedCPUMemory(), (long long)p->diagnostic_decode_max_us, (long long)p->diagnostic_present_max_us, (long long)p->diagnostic_wake_max_us);
   SbLogRaw(counters);
#ifdef MYOS_MEDIA_VP9
   {char hardware[160];snprintf(hardware,sizeof(hardware),
    "MEDIA hardware-clock reads=%u failures=%u max-ack-lag-frames=%u presentation-max-us=%lld rebuffers=%u\n",
    p->native_hardware_clock_reads,p->native_hardware_clock_failures,p->native_hardware_ack_lag_frames,(long long)p->native_presentation_max_us,p->native_rebuffers);SbLogRaw(hardware);}
#endif
#endif
  }
#endif
  if(p->seek_pending){
   int ticket=p->ticket;unsigned revision=p->seek_revision;p->presenting=0;p->ended=0;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
   if(p->native_video&&p->has_video){media_video_clear(p->video_window);p->video_shown=0;p->shown_generation=0;}
#endif
   pthread_mutex_unlock(&p->mutex);
   if(p->sink){SbAudioSinkDestroy(p->sink);p->sink=0;}
   release_pending(p);
   int error=reset_decoders(p);
   pthread_mutex_lock(&p->mutex);
   p->pcm_read=p->pcm_count=p->resample_phase=0;p->consumed=0;p->audio_anchor_set=0;
   p->video_read=p->video_count=p->have_current=0;p->duration=0;p->wall_media=p->seek_time;p->wall_anchor=now_us();
   p->last_audio_pts=p->last_video_pts=INT64_MIN;p->failed=0;
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   p->native_rebuffering=0;
#endif
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
   p->native_first_video_pending=p->native_video&&p->has_video;
#endif
   pthread_mutex_unlock(&p->mutex);
   if(!error)error=create_sink(p);
   pthread_mutex_lock(&p->mutex);
   if(error)p->error_pending=1;
   int current_revision=revision==p->seek_revision;
   if(current_revision){p->seek_pending=0;p->seeked=1;}
   pthread_mutex_unlock(&p->mutex);
   if(current_revision)report_status(p,kSbPlayerStatePrerolling,ticket);
   continue;
  }
  int ticket=p->ticket,notify_present=0,notify_end=0,notify_error=0;
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  int native_resume_ready=0;
#endif
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  const void *deallocate[2][AUDIO_BATCH]={{0}};unsigned deallocate_count[2]={0,0};
#else
  const void *deallocate[2]={0,0};
#endif
  int request[2]={0,0};
  if(p->error_pending){p->error_pending=0;p->failed=1;notify_error=1;}
  if(!p->failed&&p->seeked){
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   /* Drain only the already bounded input batch. Sleeping after each
    * compressed frame otherwise limits catch-up after a scheduling delay.
    * Retire due frames while draining so the four-slot queue stays bounded. */
   for(int i=0;i<2&&!p->failed;++i){
    struct pending *pending=&p->pending[i];
    unsigned budget=i==kSbMediaTypeAudio?AUDIO_BATCH:VIDEO_BATCH;
    while(pending->full&&budget--){
     if(i==kSbMediaTypeVideo){
      int64_t due=native_frame_due_locked(p);
      while(p->video_count&&(!p->have_current||p->frames[p->video_read].time<=due)){
       if(p->have_current&&p->generation!=p->rendered_generation)++p->dropped_video;
       p->current=p->frames[p->video_read];p->have_current=1;++p->generation;
       p->video_read=(p->video_read+1)%VIDEO_SLOTS;--p->video_count;
      }
     }
     if((i==kSbMediaTypeAudio&&PCM_CAP-p->pcm_count<1400)||(i==kSbMediaTypeVideo&&p->video_count==VIDEO_SLOTS))break;
     int error=i==kSbMediaTypeAudio?decode_audio_locked(p,&pending->samples[pending->read]):decode_video_locked(p,&pending->samples[pending->read]);
     if(pending->samples[pending->read].timestamp>p->duration)p->duration=pending->samples[pending->read].timestamp;
     deallocate[i][deallocate_count[i]++]=pending->samples[pending->read++].buffer;--pending->full;
     if(error){p->failed=1;notify_error=1;break;}
    }
   }
#else
   for(int i=0;i<2;++i)if(p->pending[i].full){
    struct pending *pending=&p->pending[i];
    if((i==kSbMediaTypeAudio&&PCM_CAP-p->pcm_count<1400)||(i==kSbMediaTypeVideo&&p->video_count==VIDEO_SLOTS))continue;
    int error=i==kSbMediaTypeAudio?decode_audio_locked(p,&pending->samples[pending->read]):decode_video_locked(p,&pending->samples[pending->read]);
    if(pending->samples[pending->read].timestamp>p->duration)p->duration=pending->samples[pending->read].timestamp;
    deallocate[i]=pending->samples[pending->read++].buffer;--pending->full;
    if(error){p->failed=1;notify_error=1;break;}
   }
#endif
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   int64_t frame_due=native_frame_due_locked(p);
#else
   int64_t clock=clock_locked(p);
   int64_t frame_due=clock+10000;
#endif
   while(p->video_count&&(!p->have_current||p->frames[p->video_read].time<=frame_due)){
    if(p->have_current&&p->generation!=p->rendered_generation)++p->dropped_video;
    p->current=p->frames[p->video_read];p->have_current=1;++p->generation;
    p->video_read=(p->video_read+1)%VIDEO_SLOTS;--p->video_count;
   }
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   int aligned_preroll=1;
   /* A seek can start video at an earlier keyframe than the audio anchor.
    * Do not start audible PCM until video has caught up and has bounded
    * scheduling headroom. EOS admits shorter, already aligned clips. */
   if(p->has_audio&&p->has_video){
    int64_t anchor=clock_locked(p);
    int64_t ahead=p->video_count?p->frames[(p->video_read+p->video_count-1)%VIDEO_SLOTS].time-anchor:0;
    aligned_preroll=p->have_current&&p->current.time>=anchor-50000&&
     (ahead>=200000||p->pending[kSbMediaTypeVideo].eos);
   }
#endif
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   if(p->native_rebuffering&&aligned_preroll&&
      (p->pcm_count>=2205||p->pending[kSbMediaTypeAudio].eos)&&native_pcm_held_locked(p)){
    /* Keep audio held through the synchronous fresh-video submission. */
    native_resume_ready=1;
   }
#endif
   if(!p->presenting&&
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
      aligned_preroll&&
#endif
      (!p->has_video||p->have_current)&&
      (!p->has_audio||p->pcm_count>=2205||p->pending[0].eos)){
    p->presenting=1;p->wall_anchor=now_us();notify_present=1;
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY) && defined(MYOS_MEDIA_TRACE)
    {char detail[192];snprintf(detail,sizeof(detail),
     "MEDIA preroll ticket=%d pts=%lld clock=%lld buffered-video=%u audio-frames=%u\n",
     ticket,(long long)p->current.time,(long long)clock_locked(p),p->video_count,p->pcm_count);SbLogRaw(detail);}
#endif
   }
   if(p->presenting&&!p->ended&&(!p->has_audio||(p->pending[0].eos&&!p->pcm_count&&!p->pending[0].full))&&
      (!p->has_video||(p->pending[1].eos&&!p->video_count&&!p->pending[1].full))){
    p->ended=1;p->rate=0;notify_end=1;
   }
   for(int i=0;i<2;++i){
    struct pending *pending=&p->pending[i];int enabled=i==0?p->has_audio:p->has_video;
    int room=i==0?PCM_CAP-p->pcm_count>=AUDIO_BATCH*1400:p->video_count<VIDEO_SLOTS;
    if(enabled&&!pending->full&&!pending->eos&&!pending->requested&&room&&!p->failed){
     pending->requested=1;request[i]=1;
    }
   }
  }
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
  if(p->native_video&&p->has_video){
   if(p->failed||!p->bounds_width||!p->bounds_height){
    if(p->video_shown){media_video_clear(p->video_window);p->video_shown=0;}
   }else if(p->have_current&&(p->generation!=p->shown_generation||p->bounds_revision!=p->shown_bounds)
#ifdef MYOS_MEDIA_VP9
     /* The sink needs this mutex to finish the async FLUSH/acknowledge.
      * Do not start another expensive presentation while DMA still runs,
      * or repaint a stale frame after the hardware clock has been held. */
     &&(!p->native_rebuffering||(native_pcm_held_locked(p)&&p->current.time>=clock_locked(p)-50000))
#endif
   ){
#if defined(MYOS_MEDIA_VP9) || defined(MYOS_MEDIA_TRACE)
    int64_t diagnostic_present_start=now_us();
#endif
    int rc=media_video_set(p->video_window,p->current.rgba,p->current.width,p->current.height,
     p->bounds_x,p->bounds_y,p->bounds_width,p->bounds_height,p->current.time);
#if defined(MYOS_MEDIA_VP9) || defined(MYOS_MEDIA_TRACE)
    int64_t diagnostic_present_duration=now_us()-diagnostic_present_start;
#ifdef MYOS_MEDIA_VP9
    if(diagnostic_present_duration>p->native_presentation_max_us)p->native_presentation_max_us=diagnostic_present_duration;
#endif
#ifdef MYOS_MEDIA_TRACE
    if(diagnostic_present_duration>p->diagnostic_present_max_us)p->diagnostic_present_max_us=diagnostic_present_duration;
#endif
#endif
    if(rc){p->failed=1;notify_error=1;}
    else {
     p->shown_generation=p->rendered_generation=p->generation;p->shown_bounds=p->bounds_revision;p->video_shown=1;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
     if(p->native_first_video_pending){
      char first[160];snprintf(first,sizeof(first),
       "MEDIA first-video ticket=%d pts=%lld clock=%lld consumed=%llu\n",
       p->ticket,(long long)p->current.time,(long long)clock_locked(p),(unsigned long long)p->consumed);SbLogRaw(first);
      p->native_first_video_pending=0;
     }
#endif
     ++p->native_presented;
     if(p->has_audio&&p->presenting&&p->rate==1.0&&p->consumed>2205){
      int64_t delta=p->current.time-clock_locked(p);if(delta<0)delta=-delta;
      if(delta>p->native_max_sync){
       p->native_max_sync=delta;
       if(delta>100000){char detail[240];snprintf(detail,sizeof(detail),
        "MEDIA sync-outlier pts=%lld clock=%lld delta=%lld pcm=%u vq=%u consumed=%llu audio-last=%lld video-last=%lld\n",
        (long long)p->current.time,(long long)clock_locked(p),(long long)delta,p->pcm_count,p->video_count,
        (unsigned long long)p->consumed,(long long)p->last_audio_pts,(long long)p->last_video_pts);SbLogRaw(detail);}
      }++p->native_sync_samples;
     }
    }
   }
  }
#endif
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  if(native_resume_ready&&!p->failed&&p->video_shown&&p->shown_generation==p->generation){
   p->native_rebuffering=0;
   char detail[160];snprintf(detail,sizeof(detail),
    "MEDIA rebuffer resume clock=%lld pts=%lld vq=%u pcm=%u\n",
    (long long)clock_locked(p),(long long)p->current.time,p->video_count,p->pcm_count);SbLogRaw(detail);
  }
#endif
  pthread_mutex_unlock(&p->mutex);
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  for(int i=0;i<2;++i)for(unsigned j=0;j<deallocate_count[i];++j)p->deallocate(p,p->context,deallocate[i][j]);
#else
  for(int i=0;i<2;++i)if(deallocate[i])p->deallocate(p,p->context,deallocate[i]);
#endif
  if(notify_error)p->error(p,p->context,kSbPlayerErrorDecode,
#if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
   p->native_hardware_clock_failures?"native audio hardware clock unavailable":
#endif
   "native VP8/Opus decode or bounded stream validation failed");
  if(notify_present)report_status(p,kSbPlayerStatePresenting,ticket);
  if(notify_end)report_status(p,kSbPlayerStateEndOfStream,ticket);
  for(int i=0;i<2;++i)if(request[i])p->decoder(p,p->context,(SbMediaType)i,kSbPlayerDecoderStateNeedsData,ticket);
#if defined(MYOS_MEDIA_TRACE) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  int64_t diagnostic_sleep_start=now_us();
#endif
  clock_nanosleep(CLOCK_MONOTONIC,0,&interval,0);
#if defined(MYOS_MEDIA_TRACE) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
  int64_t diagnostic_wake_delay=now_us()-diagnostic_sleep_start-interval.tv_nsec/1000;
  if(diagnostic_wake_delay>p->diagnostic_wake_max_us)p->diagnostic_wake_max_us=diagnostic_wake_delay;
#endif
 }
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(p->native_video&&p->has_video)media_video_clear(p->video_window);
#endif
 if(p->sink){SbAudioSinkDestroy(p->sink);p->sink=0;}
 release_pending(p);
 report_status(p,kSbPlayerStateDestroyed,p->ticket);return 0;
}
static int valid_creation(const SbPlayerCreationParam *param){
 if(!param||param->drm_system)return 0;
 int audio=param->audio_stream_info.codec!=kSbMediaAudioCodecNone;
 int video=param->video_stream_info.codec!=kSbMediaVideoCodecNone;
 if(!audio&&!video)return 0;
 if(audio&&(param->audio_stream_info.codec!=kSbMediaAudioCodecOpus||
  (param->audio_stream_info.number_of_channels!=1&&param->audio_stream_info.number_of_channels!=2)||
  param->audio_stream_info.samples_per_second!=48000))return 0;
 int supported_video=param->video_stream_info.codec==kSbMediaVideoCodecVp8;
#ifdef MYOS_MEDIA_VP9
 supported_video|=param->video_stream_info.codec==kSbMediaVideoCodecVp9;
#endif
 if(video&&(!supported_video||
  param->video_stream_info.frame_width<1||param->video_stream_info.frame_width>DECODE_W||
  param->video_stream_info.frame_height<1||param->video_stream_info.frame_height>DECODE_H))return 0;
 if(video&&param->video_stream_info.codec==kSbMediaVideoCodecVp8&&
    (param->video_stream_info.frame_width>MAX_W||param->video_stream_info.frame_height>MAX_H))return 0;
 return 1;
}
SbPlayerOutputMode SbPlayerGetPreferredOutputMode(const SbPlayerCreationParam *param){
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(valid_creation(param)&&media_video_supported())return kSbPlayerOutputModePunchOut;
#endif
#ifdef MYOS_MEDIA_PUNCHOUT_DIAGNOSTIC
 return valid_creation(param)?kSbPlayerOutputModePunchOut:kSbPlayerOutputModeInvalid;
#else
 return valid_creation(param)?kSbPlayerOutputModeDecodeToTexture:kSbPlayerOutputModeInvalid;
#endif
}
SbPlayer SbPlayerCreate(SbWindow window,const SbPlayerCreationParam *param,
 SbPlayerDeallocateSampleFunc deallocate,SbPlayerDecoderStatusFunc decoder,
 SbPlayerStatusFunc status,SbPlayerErrorFunc error,void *context,
 SbDecodeTargetGraphicsContextProvider *provider){
 (void)window;(void)provider;MEDIA_TRACE("MEDIA create entered\n");
 int allowed_output=param&&param->output_mode==kSbPlayerOutputModeDecodeToTexture;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(param&&param->output_mode==kSbPlayerOutputModePunchOut&&media_video_supported()&&
    (param->video_stream_info.codec==kSbMediaVideoCodecNone||window))allowed_output=1;
#endif
#ifdef MYOS_MEDIA_PUNCHOUT_DIAGNOSTIC
 if(param&&param->output_mode==kSbPlayerOutputModePunchOut)allowed_output=1;
#endif
 if(!valid_creation(param)||!allowed_output||
    !deallocate||!decoder||!status||!error){MEDIA_TRACE("MEDIA create invalid parameters\n");return 0;}
 #if defined(MYOS_MEDIA_VP9) && defined(MYOS_MEDIA_NATIVE_UNDERLAY)
 {char format[320];snprintf(format,sizeof(format),
  "MEDIA stream video-mime=%s audio-mime=%s video-codec=%d audio-codec=%d audio-channels=%d audio-rate=%d mode=%d\n",
  param->video_stream_info.mime?param->video_stream_info.mime:"(unspecified)",
  param->audio_stream_info.mime?param->audio_stream_info.mime:"(unspecified)",
  (int)param->video_stream_info.codec,(int)param->audio_stream_info.codec,
  param->audio_stream_info.number_of_channels,param->audio_stream_info.samples_per_second,(int)param->output_mode);SbLogRaw(format);}
#endif
 SbPlayer p=calloc(1,sizeof(*p));if(!p)return 0;
 if(pthread_mutex_init(&p->mutex,0)){free(p);return 0;}
 p->deallocate=deallocate;p->decoder=decoder;p->status=status;p->error=error;p->context=context;
 p->has_audio=param->audio_stream_info.codec!=kSbMediaAudioCodecNone;
 p->has_video=param->video_stream_info.codec!=kSbMediaVideoCodecNone;
#ifdef MYOS_MEDIA_VP9
 p->video_codec=param->video_stream_info.codec;
#endif
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 p->native_video=param->output_mode==kSbPlayerOutputModePunchOut;p->video_window=(uintptr_t)window;
 p->native_first_video_pending=p->native_video&&p->has_video;
#endif
 p->channels=param->audio_stream_info.number_of_channels;p->rate=p->volume=1;
 p->last_audio_pts=p->last_video_pts=INT64_MIN;
 pthread_mutex_lock(&registry);if(active){pthread_mutex_unlock(&registry);goto failed;}active=p;pthread_mutex_unlock(&registry);
 if(p->has_audio){int code;p->audio=opus_decoder_create(48000,p->channels,&code);if(!p->audio)goto reserved_failed;}
 if(reset_decoders(p)){MEDIA_TRACE("MEDIA decoder initialization failed\n");goto reserved_failed;}
 if(create_sink(p)){MEDIA_TRACE("MEDIA sink initialization failed\n");goto reserved_failed;}
 pthread_attr_t attr;if(pthread_attr_init(&attr))goto reserved_failed;
 int rc=pthread_attr_setstacksize(&attr,2u*1024u*1024u);
 if(!rc)rc=pthread_create(&p->worker,&attr,player_worker,p);
 pthread_attr_destroy(&attr);
#ifdef MYOS_MEDIA_TRACE
 if(rc){char message[96];snprintf(message,sizeof(message),"MEDIA worker creation failed rc=%d\n",rc);SbLogRaw(message);}
#endif
 if(rc)goto reserved_failed;
 MEDIA_TRACE("MEDIA create success\n");__atomic_store_n(&p->published,1,__ATOMIC_RELEASE);return p;
reserved_failed:
 MEDIA_TRACE("MEDIA create resource failure\n");
 if(p->sink)SbAudioSinkDestroy(p->sink);
 if(p->video_initialized)vpx_codec_destroy(&p->video);
 if(p->audio)opus_decoder_destroy(p->audio);
 pthread_mutex_lock(&registry);active=0;pthread_mutex_unlock(&registry);
failed:
 pthread_mutex_destroy(&p->mutex);free(p);return 0;
}
void SbPlayerDestroy(SbPlayer handle){
 SbPlayer p=lock_player(handle);if(!p)return;
 if(pthread_equal(pthread_self(),p->worker)){unlock_player(p);return;}
 __atomic_store_n(&p->closing,1,__ATOMIC_RELEASE);p->stop=1;unlock_player(p);
 if(pthread_join(p->worker,0))return;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(p->native_video){
  char metrics[256];snprintf(metrics,sizeof(metrics),
   "MEDIA native metrics presented=%u sync-samples=%u max-sync-us=%lld decoded=%u dropped=%u corrupted=%u source-starvations=%u\n",
   p->native_presented,p->native_sync_samples,(long long)p->native_max_sync,p->total_video,
   p->dropped_video,p->corrupted_video,p->source_starvations);SbLogRaw(metrics);
 }
#endif
 if(p->video_initialized)vpx_codec_destroy(&p->video);
 if(p->audio)opus_decoder_destroy(p->audio);
 pthread_mutex_lock(&registry);active=0;pthread_mutex_unlock(&registry);
 pthread_mutex_destroy(&p->mutex);free(p);
}
void SbPlayerSeek(SbPlayer handle,int64_t time,int ticket){
#ifdef MYOS_MEDIA_TRACE
 char message[96];snprintf(message,sizeof(message),"MEDIA seek ticket=%d target-us=%lld\n",ticket,(long long)time);SbLogRaw(message);
#endif
 SbPlayer p=lock_player(handle);if(!p)return;
 if(time<0){p->error_pending=1;unlock_player(p);return;}
 p->seek_time=time;p->ticket=ticket;++p->seek_revision;p->seek_pending=1;p->presenting=0;
 unlock_player(p);
}
void SbPlayerWriteSamples(SbPlayer handle,SbMediaType type,const SbPlayerSampleInfo *s,int count){
 SbPlayer p=lock_player(handle);if(!p)return;
 int enabled=type==kSbMediaTypeAudio?p->has_audio:type==kSbMediaTypeVideo?p->has_video:0;
 int limit=type==kSbMediaTypeAudio?AUDIO_BATCH:VIDEO_BATCH;
 int invalid=!enabled||count<1||count>limit||!s||p->seek_pending;
 if(!invalid)invalid=p->pending[type].full||!p->pending[type].requested;
 if(!invalid)for(int i=0;i<count;++i){
  if(s[i].type!=type||!s[i].buffer||s[i].buffer_size<=0||s[i].buffer_size>MAX_PACKET||
     s[i].drm_info||s[i].side_data_count){invalid=1;break;}
 }
 if(invalid){
  p->error_pending=1;
  const void *rejected[AUDIO_BATCH];unsigned n=0;
  if(s&&count>0){int bounded=count>AUDIO_BATCH?1:count;for(int i=0;i<bounded;++i)if(s[i].buffer)rejected[n++]=s[i].buffer;}
  SbPlayerDeallocateSampleFunc callback=p->deallocate;void *context=p->context;
  unlock_player(p);for(unsigned i=0;i<n;++i)callback(handle,context,rejected[i]);return;
 }
 struct pending *pending=&p->pending[type];
 for(int i=0;i<count;++i)pending->samples[i]=s[i];
 pending->read=0;pending->full=count;pending->requested=0;unlock_player(p);
}
int SbPlayerGetMaximumNumberOfSamplesPerWrite(SbPlayer handle,SbMediaType type){
 SbPlayer p=lock_player(handle);if(!p)return 0;int valid=type==0?p->has_audio:type==1?p->has_video:0;
 unlock_player(p);return valid?(type==kSbMediaTypeAudio?AUDIO_BATCH:VIDEO_BATCH):0;
}
void SbPlayerWriteEndOfStream(SbPlayer handle,SbMediaType type){
 SbPlayer p=lock_player(handle);if(!p)return;
 if((type!=0&&type!=1)||p->pending[type].full||!p->pending[type].requested||p->seek_pending)p->error_pending=1;
 else {p->pending[type].eos=1;p->pending[type].requested=0;}
 unlock_player(p);
}
bool SbPlayerSetPlaybackRate(SbPlayer handle,double rate){
 SbPlayer p=lock_player(handle);if(!p)return false;
 if(rate!=0.0&&rate!=1.0){unlock_player(p);return false;}
 p->wall_media=clock_locked(p);p->wall_anchor=now_us();p->rate=rate;unlock_player(p);return true;
}
void SbPlayerSetVolume(SbPlayer handle,double volume){
 SbPlayer p=lock_player(handle);if(!p)return;
 if(volume>=0.0&&volume<=1.0)p->volume=volume;else p->error_pending=1;unlock_player(p);
}
void SbPlayerSetBounds(SbPlayer handle,int z,int x,int y,int width,int height){
 (void)z;(void)x;(void)y;(void)width;(void)height;
#ifdef MYOS_MEDIA_TRACE
 char msg[150];snprintf(msg,sizeof(msg),"MEDIA bounds z=%d x=%d y=%d width=%d height=%d\n",z,x,y,width,height);SbLogRaw(msg);
#endif
 SbPlayer p=lock_player(handle);if(!p)return;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(p->native_video){
  if(width<0||height<0||width>4096||height>4096||x < -8192||x>8192||y < -8192||y>8192)p->error_pending=1;
  else {p->bounds_x=x;p->bounds_y=y;p->bounds_width=width;p->bounds_height=height;++p->bounds_revision;}
 }
#endif
 unlock_player(p);
 /* Decode-to-texture bounds are applied by the engine compositor. */
}
void SbPlayerGetInfo(SbPlayer handle,SbPlayerInfo *out){
 if(!out)return;SbPlayer p=lock_player(handle);if(!p)return;
 *out=(SbPlayerInfo){.current_media_timestamp=clock_locked(p),.duration=p->duration,
  .frame_width=p->have_current?p->current.width:0,.frame_height=p->have_current?p->current.height:0,
  .is_paused=p->rate==0.0,.volume=p->volume,.total_video_frames=(int)p->total_video,
  .dropped_video_frames=(int)p->dropped_video,.corrupted_video_frames=(int)p->corrupted_video,.playback_rate=p->rate};
 unlock_player(p);
}
bool SbPlayerGetAudioConfiguration(SbPlayer handle,int index,SbMediaAudioConfiguration *out){
 SbPlayer p=lock_player(handle);if(!p)return false;int audio=p->has_audio;unlock_player(p);
 return audio&&SbMediaGetAudioConfiguration(index,out);
}
SbDecodeTarget SbPlayerGetCurrentFrame(SbPlayer handle){
 SbPlayer p=lock_player(handle);if(!p)return 0;
#ifdef MYOS_MEDIA_NATIVE_UNDERLAY
 if(p->native_video){unlock_player(p);return 0;}
#endif
#ifdef MYOS_MEDIA_TRACE
 static unsigned pulls;unsigned pull=++pulls;
 if(pull<=10||pull%100==0){char msg[180];snprintf(msg,sizeof(msg),"MEDIA pull=%u current=%u seek=%d generation=%u targets=%u\n",pull,p->have_current,p->seek_pending,p->generation,target_count);SbLogRaw(msg);}
#endif
 if(!p->have_current||p->seek_pending){unlock_player(p);return 0;}
 /* Choose the nearest frame again at the renderer pull. The decoder worker
  * can have been descheduled while the compositor prepares its draw. A
  * bounded half-frame lead compensates presentation work without a second
  * wall clock; pause keeps the existing frame fixed. */
 if(p->presenting&&p->rate==1.0){
  int64_t clock=clock_locked(p);
  while(p->video_count&&p->frames[p->video_read].time<=clock+30000){
   if(p->generation!=p->rendered_generation)++p->dropped_video;
   p->current=p->frames[p->video_read];++p->generation;
   p->video_read=(p->video_read+1)%VIDEO_SLOTS;--p->video_count;
  }
 }
 pthread_mutex_lock(&targets_mutex);
 if(target_count==TARGET_LIMIT){pthread_mutex_unlock(&targets_mutex);unlock_player(p);return 0;}
 SbDecodeTarget t=calloc(1,sizeof(*t));if(!t){pthread_mutex_unlock(&targets_mutex);unlock_player(p);return 0;}
 const SbGlesInterface *gl=SbGetGlesInterface();
 int old_binding=0,old_alignment=4;
 gl->glGetIntegerv(0x8069,&old_binding);gl->glGetIntegerv(0x0cf5,&old_alignment);
 unsigned texture=0;gl->glGenTextures(1,&texture);gl->glBindTexture(0x0de1,texture);
 gl->glPixelStorei(0x0cf5,1);
 gl->glTexParameteri(0x0de1,0x2801,0x2601);gl->glTexParameteri(0x0de1,0x2800,0x2601);
 gl->glTexParameteri(0x0de1,0x2802,0x812f);gl->glTexParameteri(0x0de1,0x2803,0x812f);
 gl->glTexImage2D(0x0de1,0,0x1908,p->current.width,p->current.height,0,0x1908,0x1401,p->current.rgba);
 gl->glBindTexture(0x0de1,(unsigned)old_binding);gl->glPixelStorei(0x0cf5,old_alignment);
 unsigned gl_error=gl->glGetError();
#ifdef MYOS_MEDIA_TRACE
 if(pull<=10||pull%100==0||gl_error){char msg[180];snprintf(msg,sizeof(msg),"MEDIA texture pull=%u id=%u error=%u size=%dx%d\n",pull,texture,gl_error,p->current.width,p->current.height);SbLogRaw(msg);}
#endif
 if(!texture||gl_error){if(texture)gl->glDeleteTextures(1,&texture);free(t);t=0;}
 else {
  t->info.format=kSbDecodeTargetFormat1PlaneRGBA;t->info.is_opaque=true;
  t->info.width=p->current.width;t->info.height=p->current.height;
  t->info.planes[0]=(SbDecodeTargetInfoPlane){texture,0x0de1,0x1908,p->current.width,p->current.height,
   {0,0,(float)p->current.width,(float)p->current.height}};
  t->next=targets;targets=t;++target_count;p->rendered_generation=p->generation;
 }
 pthread_mutex_unlock(&targets_mutex);unlock_player(p);return t;
}
bool SbDecodeTargetGetInfo(SbDecodeTarget target,SbDecodeTargetInfo *out){
 if(!out)return false;pthread_mutex_lock(&targets_mutex);
 SbDecodeTarget t=targets;while(t&&t!=target)t=t->next;
 if(t)*out=t->info;pthread_mutex_unlock(&targets_mutex);return t!=0;
}
void SbDecodeTargetRelease(SbDecodeTarget target){
 pthread_mutex_lock(&targets_mutex);SbDecodeTarget *p=&targets;
 while(*p&&*p!=target)p=&(*p)->next;
 if(*p){SbDecodeTarget t=*p;*p=t->next;--target_count;
  SbGetGlesInterface()->glDeleteTextures(1,&t->info.planes[0].texture);free(t);}
 pthread_mutex_unlock(&targets_mutex);
}
