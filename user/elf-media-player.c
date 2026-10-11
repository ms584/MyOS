/* Native paired player checkpoint; two bounded guest demux passes for asset only.
 * Each track reads the same asset independently to avoid interleaving head-of-line
 * blocking. Does NOT establish Cobalt demux/HTTPS media acceptance. */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include "starboard/configuration.h"
#include "starboard/player.h"
#include "starboard/egl.h"
#include "starboard/gles.h"
#include "starboard/window.h"
#include "starboard/log.h"
#include "media-webm.h"
#ifdef MYOS_TEST_VP9
#define TEST_VIDEO_CODEC kSbMediaVideoCodecVp9
#define TEST_VIDEO_MIME "video/webm; codecs=vp9"
#else
#define TEST_VIDEO_CODEC kSbMediaVideoCodecVp8
#define TEST_VIDEO_MIME "video/webm; codecs=vp8"
#endif
extern int64_t SbSystemGetUsedCPUMemory(void);
static int64_t ram_baseline,ram_peak;static uint64_t ram_free_baseline,ram_free_min;
static uint64_t free_pages(void){
 register uint64_t a __asm__("rax")=50;
 __asm__ volatile("int $0x80":"+a"(a):"D"((uint64_t)1),"S"((uint64_t)0),"d"((uint64_t)0):"memory","cc");return a;
}
static void sample_ram(void){
 int64_t bytes=SbSystemGetUsedCPUMemory();if(bytes>ram_peak)ram_peak=bytes;
 uint64_t pages=free_pages();if(pages<ram_free_min)ram_free_min=pages;
}
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;
static SbPlayer player;static SbWindow window;
static int requested[2],ticket=1,failed,ended,stop_readers,preroll,initialized;
static unsigned allocated,released,packets[2];
#ifdef MYOS_TEST_VIDEO_GAP
static int video_gap_done;
#endif
static int64_t seek_to;
static pthread_t readers[2];
static const SbGlesInterface *gl;
static const SbEglInterface *egl;
static void lognum(const char *label,int64_t n){char s[160];snprintf(s,sizeof(s),"%s%lld\n",label,(long long)n);SbLogRaw(s);}
static int64_t now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (int64_t)t.tv_sec*1000000+t.tv_nsec/1000;}
static void sleep_ms(int ms){struct timespec t={ms/1000,(ms%1000)*1000000};clock_nanosleep(CLOCK_MONOTONIC,0,&t,0);}
static int bad(const char *s){SbLogRaw(s);SbLogRaw("\n");pthread_mutex_lock(&mu);failed=1;pthread_mutex_unlock(&mu);return 1;}
static void dealloc(SbPlayer p,void *ctx,const void *data){(void)p;(void)ctx;free((void *)data);pthread_mutex_lock(&mu);++released;pthread_mutex_unlock(&mu);}
static void decoder(SbPlayer p,void *ctx,SbMediaType type,SbPlayerDecoderState state,int t){
 (void)p;(void)ctx;if(state!=kSbPlayerDecoderStateNeedsData)return;
 pthread_mutex_lock(&mu);if(t==ticket)requested[type]=1;pthread_mutex_unlock(&mu);
}
static void status(SbPlayer p,void *ctx,SbPlayerState state,int t){
 (void)p;(void)ctx;lognum("native player state=",state);
 pthread_mutex_lock(&mu);if(state==kSbPlayerStateInitialized)initialized=1;
 if(t==ticket){if(state==kSbPlayerStatePrerolling)preroll=1;if(state==kSbPlayerStateEndOfStream)ended=1;}pthread_mutex_unlock(&mu);
}
static void error(SbPlayer p,void *ctx,SbPlayerError e,const char *message){(void)p;(void)ctx;lognum("native player ERROR=",e);bad(message);}
static int pull(void *ctx,void *buffer,size_t n){
 int fd=*(int *)ctx;unsigned char *p=buffer;
 while(n){ssize_t k=read(fd,p,n>4096?4096:n);if(k<=0||(size_t)k>n)return -1;p+=k;n-=(size_t)k;}return 0;
}
struct track {int kind;int64_t skip;int started;};
static int wait_request(int type){
 for(;;){pthread_mutex_lock(&mu);
  int stop=stop_readers||failed;int ready=requested[type];
  if(ready&&!stop)requested[type]=0;pthread_mutex_unlock(&mu);
  if(stop)return -1;if(ready)return 0;sleep_ms(2);
 }
}
static int packet(void *context,int kind,const unsigned char *data,size_t n,int64_t stamp,int key){
 struct track *track=context;if(kind!=track->kind)return 0;
 int type=kind==1?kSbMediaTypeVideo:kSbMediaTypeAudio;
 if(!track->started){
  if(stamp<track->skip||(kind==1&&!key))return 0;track->started=1;
 }
 if(wait_request(type))return 1;
#ifdef MYOS_TEST_VIDEO_GAP
 if(type==kSbMediaTypeVideo&&!video_gap_done&&stamp>=8000000){
  video_gap_done=1;lognum("native VIDEO_GAP begin pts=",stamp);
  sleep_ms(1200);SbPlayerInfo before={0};SbPlayerGetInfo(player,&before);
  sleep_ms(600);SbPlayerInfo after={0};SbPlayerGetInfo(player,&after);
  lognum("native VIDEO_GAP held-before=",before.current_media_timestamp);
  lognum("native VIDEO_GAP held-after=",after.current_media_timestamp);
  if(after.current_media_timestamp-before.current_media_timestamp>100000)
   return bad("native VIDEO_GAP audio advanced while video withheld");
  SbLogRaw("native VIDEO_GAP held clock PASS; feeder restored\n");
 }
#endif
 void *copy=malloc(n);if(!copy)return bad("native fixture packet allocation failure");
 memcpy(copy,data,n);SbPlayerSampleInfo sample={0};
 sample.type=type;sample.buffer=copy;sample.buffer_size=(int)n;sample.timestamp=stamp;
 if(type==kSbMediaTypeAudio){
  sample.audio_sample_info.stream_info.codec=kSbMediaAudioCodecOpus;
  sample.audio_sample_info.stream_info.number_of_channels=1;
  sample.audio_sample_info.stream_info.samples_per_second=48000;
  /* Generated WebM declares CodecDelay=6500us. */
  if(stamp==0)sample.audio_sample_info.discarded_duration_from_front=6500;
 }else{
  sample.video_sample_info.stream_info.codec=TEST_VIDEO_CODEC;
  sample.video_sample_info.stream_info.frame_width=160;sample.video_sample_info.stream_info.frame_height=90;
 }
 pthread_mutex_lock(&mu);++allocated;++packets[type];pthread_mutex_unlock(&mu);
 SbPlayerWriteSamples(player,(SbMediaType)type,&sample,1);return 0;
}
static void *read_track(void *which){
 struct track t={(int)(uintptr_t)which,seek_to,0};
 int fd=open("/assets/media",O_RDONLY);if(fd<0){bad("native fixture asset open");return 0;}
 off_t len=lseek(fd,0,SEEK_END);lseek(fd,0,SEEK_SET);
 struct myos_webm_source source={pull,&fd,(uint64_t)len};
 int result=myos_webm_demux(&source,packet,&t);close(fd);
 pthread_mutex_lock(&mu);int stopped=stop_readers;pthread_mutex_unlock(&mu);
 if(!stopped&&result)bad("native fixture demux failed");
 if(!stopped&&!result){int type=t.kind==1?kSbMediaTypeVideo:kSbMediaTypeAudio;
  if(!wait_request(type))SbPlayerWriteEndOfStream(player,(SbMediaType)type);
 }
 return 0;
}
static int start_readers(void){
 pthread_attr_t a;if(pthread_attr_init(&a)||pthread_attr_setstacksize(&a,2u*1024u*1024u))return 1;
 for(int i=0;i<2;++i)if(pthread_create(&readers[i],&a,read_track,(void *)(uintptr_t)(i+1)))return 1;
 pthread_attr_destroy(&a);return 0;
}
static void stop_feeders(void){
 pthread_mutex_lock(&mu);stop_readers=1;pthread_mutex_unlock(&mu);
 pthread_join(readers[0],0);pthread_join(readers[1],0);
}
static unsigned shader(unsigned type,const char *source){
 unsigned s=gl->glCreateShader(type);gl->glShaderSource(s,1,&source,0);gl->glCompileShader(s);
 int ok=0;gl->glGetShaderiv(s,0x8b81,&ok);if(!ok)bad("native fixture shader failed");return s;
}
static void *run(void *unused){
 (void)unused;gl=SbGetGlesInterface();egl=SbGetEglInterface();
 SbWindowOptions options;SbWindowSetDefaultOptions(&options);options.name="MyOS standard VP8 + Opus";
 window=SbWindowCreate(&options);if(!window)return (void *)1;
 SbEglDisplay display=egl->eglGetDisplay(0);int major,minor;
 if(!egl->eglInitialize(display,&major,&minor)||!egl->eglBindAPI(0x30a0))return (void *)2;
 const int attrs[]={0x3040,4,0x3033,4,0x3024,8,0x3038};
 SbEglConfig config;int count;if(!egl->eglChooseConfig(display,attrs,&config,1,&count)||count!=1)return (void *)3;
 const int ctxattrs[]={0x3098,2,0x3038};
 SbEglContext ctx=egl->eglCreateContext(display,config,0,ctxattrs);
 SbEglSurface surf=egl->eglCreateWindowSurface(display,config,(intptr_t)SbWindowGetPlatformHandle(window),0);
 if(!ctx||!surf||!egl->eglMakeCurrent(display,surf,surf,ctx))return (void *)4;
 unsigned vs=shader(0x8b31,"attribute vec2 p;attribute vec2 uv;varying vec2 v;void main(){gl_Position=vec4(p,0.,1.);v=uv;}");
 unsigned fs=shader(0x8b30,"precision mediump float;varying vec2 v;uniform sampler2D image;void main(){gl_FragColor=texture2D(image,v);}");
 unsigned program=gl->glCreateProgram();gl->glAttachShader(program,vs);gl->glAttachShader(program,fs);
 gl->glBindAttribLocation(program,0,"p");gl->glBindAttribLocation(program,1,"uv");gl->glLinkProgram(program);gl->glUseProgram(program);
 gl->glUniform1i(gl->glGetUniformLocation(program,"image"),0);
 const float vertices[]={-1,-1,0,1, 1,-1,1,1, -1,1,0,0, 1,1,1,0};
 gl->glVertexAttribPointer(0,2,0x1406,0,4*sizeof(float),vertices);
 gl->glVertexAttribPointer(1,2,0x1406,0,4*sizeof(float),vertices+2);
 gl->glEnableVertexAttribArray(0);gl->glEnableVertexAttribArray(1);gl->glViewport(80,80,320,180);
 SbPlayerCreationParam param={0};param.audio_stream_info.codec=kSbMediaAudioCodecOpus;
 param.audio_stream_info.number_of_channels=1;param.audio_stream_info.samples_per_second=48000;
 param.audio_stream_info.mime="audio/webm; codecs=opus";
 param.video_stream_info.codec=TEST_VIDEO_CODEC;param.video_stream_info.frame_width=160;param.video_stream_info.frame_height=90;
 param.video_stream_info.mime=TEST_VIDEO_MIME;param.output_mode=kSbPlayerOutputModeDecodeToTexture;
#ifdef MYOS_TEST_UNDERLAY
 param.output_mode=kSbPlayerOutputModePunchOut;
 gl->glClearColor(0,0,0,0);gl->glClear(0x4000);
 if(!egl->eglSwapBuffers(display,surf))return (void *)4;
#endif
 if(SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=\"vp8, opus\"; width=160; height=90; framerate=15","")!=kSbMediaSupportTypeProbably||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=\"vp9, opus\"","")!=
#ifdef MYOS_TEST_VP9
    kSbMediaSupportTypeProbably
#else
    kSbMediaSupportTypeNotSupported
#endif
    ||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp8; width=161","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp8","drm")!=kSbMediaSupportTypeNotSupported)
  return (void *)20;
#ifdef MYOS_TEST_VP9
 if(SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp09.00.51.08.01.01.01.01.00; width=320; height=240; framerate=15","")!=kSbMediaSupportTypeProbably||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp09.00.51.08.01.01.01.01.00; width=192; height=144; framerate=15","")!=kSbMediaSupportTypeProbably||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp9; width=641","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp9; height=361","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp9; framerate=30.1","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp09.02.51.10.01.09.16.09.00","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp09.00.51.08.01.01.01.01.01","")!=kSbMediaSupportTypeNotSupported||
    SbMediaCanPlayMimeAndKeySystem("video/webm; codecs=vp09.00.51.08.01.01.01.01","")!=kSbMediaSupportTypeNotSupported)
  return (void *)20;
#endif
 ram_baseline=SbSystemGetUsedCPUMemory();ram_peak=ram_baseline;
 ram_free_baseline=ram_free_min=free_pages();
 player=SbPlayerCreate(window,&param,dealloc,decoder,status,error,0,0);
 if(!player)return (void *)5;
#ifdef MYOS_TEST_UNDERLAY
#ifndef MYOS_TEST_LATE_BOUNDS
 SbPlayerSetBounds(player,0,80,340,320,180);
#endif
#endif
 if(SbPlayerCreate(window,&param,dealloc,decoder,status,error,0,0)||
    SbPlayerSetPlaybackRate(player,2.0)||SbPlayerGetCurrentFrame(player))return (void *)21;
 SbPlayerGetInfo((SbPlayer)(uintptr_t)1,0);SbPlayerDestroy((SbPlayer)(uintptr_t)1);
 SbDecodeTargetInfo invalid={0};
 if(SbDecodeTargetGetInfo((SbDecodeTarget)(uintptr_t)1,&invalid))return (void *)22;
 SbLogRaw("native capability/invalid handles/player capacity PASS\n");
 for(int i=0;i<1000&&!initialized;++i)sleep_ms(10);
 SbPlayerSeek(player,0,ticket);
 if(start_readers())return (void *)6;
 int64_t start=now(),pause_stamp=0;int paused=0,resumed=0,sought=0,done=0;
 unsigned draws=0;int64_t last_log=-1;
#ifdef MYOS_TEST_LATE_BOUNDS
 int bounds_released=0;
#endif
#ifndef MYOS_TEST_UNDERLAY
 unsigned sync_samples=0;int64_t max_sync=0;
#endif
#ifndef MYOS_TEST_UNDERLAY
 int64_t seek_wall=now();
#endif
 SbLogRaw("native standard player RUN begin\n");
 while(now()-start<90000000&&!failed&&!done){
  sample_ram();
  SbPlayerInfo info={0};SbPlayerGetInfo(player,&info);
  int64_t elapsed=now()-start;
#ifdef MYOS_TEST_LATE_BOUNDS
  if(!bounds_released&&elapsed>=2000000){
   lognum("native LATE_BOUNDS held-clock=",info.current_media_timestamp);
   if(info.current_media_timestamp>100000)bad("native LATE_BOUNDS audio advanced before first video");
   else SbLogRaw("native LATE_BOUNDS clock held until visible bounds PASS\n");
   SbPlayerSetBounds(player,0,80,340,320,180);bounds_released=1;
  }
#endif
  if(!paused&&info.current_media_timestamp>=5000000){
   SbPlayerSetPlaybackRate(player,0);pause_stamp=info.current_media_timestamp;paused=1;
   lognum("native PAUSE position=",pause_stamp);start=now()-6000000;
  }else if(paused&&!resumed&&elapsed>=8000000){
   if(info.current_media_timestamp-pause_stamp>100000)bad("native PAUSE clock moved");
   SbPlayerSetPlaybackRate(player,1);resumed=1;SbLogRaw("native RESUME\n");
  }
  if(resumed&&!sought&&info.current_media_timestamp>=10000000){
   stop_feeders();pthread_mutex_lock(&mu);++ticket;requested[0]=requested[1]=preroll=ended=0;stop_readers=0;seek_to=15000000;pthread_mutex_unlock(&mu);
   SbPlayerSeek(player,seek_to,ticket);
    /* HTML media caches GetInfo before asynchronous seek teardown completes.
     * It must observe the new timeline, never the previous audio anchor. */
    SbPlayerInfo seek_info={0};SbPlayerGetInfo(player,&seek_info);
    lognum("native immediate SEEK position=",seek_info.current_media_timestamp);
    if(seek_info.current_media_timestamp!=seek_to)bad("native pending SEEK reported old timeline");
    if(start_readers())bad("native seek feeders failed");
   sought=1;
#ifndef MYOS_TEST_UNDERLAY
   seek_wall=now();
#endif
   SbLogRaw("native SEEK to=15000000\n");
  }
  #ifndef MYOS_TEST_UNDERLAY
#ifdef MYOS_TEST_VP9
  int64_t frame_began=now();
#endif
  SbDecodeTarget target=SbPlayerGetCurrentFrame(player);
  if(target){SbDecodeTargetInfo ti={0};if(!SbDecodeTargetGetInfo(target,&ti)||ti.width!=160||ti.height!=90)bad("native texture dimensions");
   gl->glClearColor(.05f,.05f,.08f,1);gl->glClear(0x4000);
   gl->glBindTexture(0x0de1,ti.planes[0].texture);gl->glDrawArrays(5,0,4);
   unsigned char cells[320*4];
   gl->glReadPixels(80,244,320,1,0x1908,0x1401,cells);
   unsigned source_frame=0;
   for(unsigned bit=0;bit<10;++bit){
    unsigned at=(bit*30+14)*4;
    /* Y235 versusY16 encoded cells remain separated after chroma conversion. */
    unsigned brightness=(unsigned)cells[at]+cells[at+1]+cells[at+2];
    if(brightness>350)source_frame|=1u<<bit;
   }
   if(!egl->eglSwapBuffers(display,surf)||gl->glGetError())bad("native texture render failed");
   SbPlayerInfo consumed={0};SbPlayerGetInfo(player,&consumed);
   if(consumed.playback_rate==1.0&&now()-seek_wall>1000000&&source_frame<600){
    int64_t video_stamp=(int64_t)source_frame*1000000/15;
    int64_t delta=consumed.current_media_timestamp-video_stamp;if(delta<0)delta=-delta;
    if(delta>max_sync){
#ifdef MYOS_TEST_VP9
     if(delta>80000){char message[192];
      snprintf(message,sizeof(message),"native VP9 sync outlier frame=%u clock=%lld delta=%lld texture-render-us=%lld\n",
               source_frame,(long long)consumed.current_media_timestamp,(long long)delta,(long long)(now()-frame_began));SbLogRaw(message);}
#endif
     max_sync=delta;}++sync_samples;
   }
   SbDecodeTargetRelease(target);++draws;
  }
#else
  ++draws;
#endif
  if(info.current_media_timestamp/1000000!=last_log){last_log=info.current_media_timestamp/1000000;lognum("native media position-us=",info.current_media_timestamp);}
  pthread_mutex_lock(&mu);done=ended;pthread_mutex_unlock(&mu);sleep_ms(30);
 }
 stop_feeders();
#ifndef MYOS_TEST_UNDERLAY
 SbDecodeTarget held[8];
 for(int i=0;i<8;++i){held[i]=SbPlayerGetCurrentFrame(player);if(!held[i])bad("native target capacity before limit");}
 if(SbPlayerGetCurrentFrame(player))bad("native target limit not enforced");
 sample_ram();
 SbPlayerDestroy(player);player=0;
 for(int i=0;i<8;++i){SbDecodeTargetInfo ti={0};if(!SbDecodeTargetGetInfo(held[i],&ti))bad("native retained target died with player");SbDecodeTargetRelease(held[i]);}
 SbLogRaw("native retained textures/player destroy/capacity PASS\n");
#else
 SbPlayerDestroy(player);player=0;
#endif
#ifndef MYOS_TEST_UNDERLAY
 lognum("native rendered textures=",draws);lognum("native presented A-V max-us=",max_sync);lognum("native presented A-V samples=",sync_samples);
#else
 lognum("native underlay control polls=",draws);
#endif
 lognum("native sample allocated=",allocated);lognum("native sample released=",released);
 int ok=!failed&&done&&paused&&resumed&&sought&&draws>100&&allocated==released;
#ifndef MYOS_TEST_UNDERLAY
 ok=ok&&sync_samples>100&&max_sync<=100000;
#endif
 /* A second native player in the SAME process proves stop/restart ownership,
  * independent of automatic process teardown. Stop it while still playing. */
 pthread_mutex_lock(&mu);++ticket;initialized=preroll=ended=stop_readers=0;
 requested[0]=requested[1]=0;seek_to=0;pthread_mutex_unlock(&mu);
 player=SbPlayerCreate(window,&param,dealloc,decoder,status,error,0,0);
 if(!player){bad("native same-process restart failed");ok=0;}
 else{
#ifdef MYOS_TEST_UNDERLAY
  SbPlayerSetBounds(player,0,80,340,320,180);
#endif
  for(int i=0;i<1000&&!initialized;++i)sleep_ms(10);
  SbPlayerSeek(player,0,ticket);
  if(start_readers()){bad("native restart feeders");ok=0;}
  else{
   int progressed=0;int64_t deadline=now()+15000000;
   while(now()<deadline&&!failed){
    sample_ram();
    SbPlayerInfo restart={0};SbPlayerGetInfo(player,&restart);
    if(restart.current_media_timestamp>=1500000&&restart.total_video_frames>=15){progressed=1;break;}
    sleep_ms(10);
   }
   stop_feeders();if(!progressed){bad("native restarted clock failed");ok=0;}
  }
  SbPlayerDestroy(player);player=0;
  if(allocated!=released){bad("native restarted sample leak");ok=0;}
 }
 sample_ram();
 lognum("native RAM process baseline-bytes=",ram_baseline);
 lognum("native RAM process sampled-peak-bytes=",ram_peak);
 lognum("native RAM player increment-bytes=",ram_peak-ram_baseline);
 lognum("native RAM PMM increment-bytes=",(int64_t)((ram_free_baseline-ram_free_min)*4096));
 if(ok)SbLogRaw("native same-process restart/stop while playing PASS\n");
 gl->glDeleteProgram(program);gl->glDeleteShader(vs);gl->glDeleteShader(fs);
 egl->eglMakeCurrent(display,0,0,0);egl->eglDestroySurface(display,surf);egl->eglDestroyContext(display,ctx);
 egl->eglTerminate(display);egl->eglReleaseThread();SbWindowDestroy(window);
 if(ok&&!failed)SbLogRaw("native standard player asset/control/EOS checkpoint PASS; NOT Cobalt/HTTPS acceptance\n");
 return (void *)(uintptr_t)(ok&&!failed?97:7);
}
int _start(void){
 pthread_attr_t attr;pthread_t thread;void *result=0;
 if(pthread_attr_init(&attr)||pthread_attr_setstacksize(&attr,2u*1024u*1024u))return 10;
 int err=pthread_create(&thread,&attr,run,0);pthread_attr_destroy(&attr);
 if(err||pthread_join(thread,&result))return 11;return (int)(uintptr_t)result;
}
