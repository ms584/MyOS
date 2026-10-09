/* Native codec checkpoint: asset reads, WebM demux and VP8/Opus decode only.
 * No playback/Starboard/30-second acceptance claim. */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "starboard/log.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"
#include "opus.h"
#include "media-webm.h"
#ifndef MEDIA_PATH
#define MEDIA_PATH "/assets/media"
#endif
#ifndef EXPECT_ERROR
#define EXPECT_ERROR 0
#endif
struct state {vpx_codec_ctx_t video;OpusDecoder *audio;unsigned frames,packets,samples,signal;uint64_t hash;int64_t last[3];};
static void number(uint64_t n){char text[32];unsigned k=0;do{text[k++]=(char)('0'+n%10);n/=10;}while(n);char out[33];for(unsigned i=0;i<k;++i)out[i]=text[k-1-i];out[k]=0;SbLogRaw(out);}
static int pull(void *context,void *buffer,size_t n){
 int fd=*(int *)context;unsigned char *p=buffer;
 while(n){ssize_t got=read(fd,p,n>4096?4096:n);if(got<=0||(size_t)got>n)return -1;p+=got;n-=(size_t)got;}return 0;
}
static int packet(void *context,int type,const unsigned char *data,size_t bytes,int64_t us,int key){
 struct state *s=context;(void)key;
 if(us<s->last[type])return 1;s->last[type]=us;
 if(type==1){
  if(!bytes)return 6;
  if(!(data[0]&1)){
   if(bytes<10||data[3]!=0x9d||data[4]!=1||data[5]!=0x2a)return 7;
   unsigned w=((unsigned)data[6]|((unsigned)data[7]<<8))&16383;
   unsigned h=((unsigned)data[8]|((unsigned)data[9]<<8))&16383;
   if(!w||!h||w>720||h>480)return 8;
  }
  if(vpx_codec_decode(&s->video,data,(unsigned)bytes,0,0)){SbLogRaw("VP8 decode error: ");SbLogRaw(vpx_codec_error(&s->video));SbLogRaw("\n");return 2;}
  vpx_codec_iter_t iter=0;vpx_image_t *image;
  while((image=vpx_codec_get_frame(&s->video,&iter))){
   if(image->d_w>720||image->d_h>480||image->fmt!=VPX_IMG_FMT_I420){SbLogRaw("VP8 output dimension/format rejected ");number(image->d_w);SbLogRaw("x");number(image->d_h);SbLogRaw("\n");return 3;}
   for(unsigned y=0;y<image->d_h;++y)for(unsigned x=0;x<image->d_w;++x)
    s->hash=(s->hash^image->planes[0][y*image->stride[0]+x])*UINT64_C(1099511628211);
   ++s->frames;
  }
 }else{
  int16_t pcm[5760*2];int count=opus_decode(s->audio,data,(int)bytes,pcm,5760,0);
  if(count<0){SbLogRaw("Opus decode error\n");return 4;}s->samples+=(unsigned)count;++s->packets;
  for(int i=0;i<count*2;++i)if(pcm[i]>500||pcm[i]<-500)++s->signal;
 }
 return 0;
}
static int decode(void){
 int fd=open(MEDIA_PATH,O_RDONLY);if(fd<0)return 1;
 off_t length=lseek(fd,0,SEEK_END);if(length<=0||lseek(fd,0,SEEK_SET)){close(fd);return 2;}
 struct state s={0};s.last[1]=s.last[2]=INT64_MIN;s.hash=UINT64_C(1469598103934665603);
 vpx_codec_dec_cfg_t cfg={1,720,480};
 if(vpx_codec_dec_init(&s.video,vpx_codec_vp8_dx(),&cfg,0)){close(fd);return 3;}
 int error=0;s.audio=opus_decoder_create(48000,2,&error);
 if(!s.audio){vpx_codec_destroy(&s.video);close(fd);return 4;}
 struct myos_webm_source source={pull,&fd,(uint64_t)length};
 int result=myos_webm_demux(&source,packet,&s);
 opus_decoder_destroy(s.audio);vpx_codec_destroy(&s.video);close(fd);
 SbLogRaw("native webm decode result=");number((uint64_t)(int64_t)result);
 SbLogRaw(" frames=");number(s.frames);SbLogRaw(" audio-packets=");number(s.packets);
 SbLogRaw(" samples=");number(s.samples);SbLogRaw(" signal=");number(s.signal);
 SbLogRaw(" y-hash=");number(s.hash);SbLogRaw("\n");
 if(EXPECT_ERROR){
  if(result!=EXPECT_ERROR)return 6;
  SbLogRaw("native standard WebM malformed/truncated rejection PASS; NOT playback\n");return 99;
 }
 if(result||s.frames<30||s.packets<30||s.samples<48000||s.signal<1000)return 5;
 SbLogRaw("native standard WebM VP8/Opus asset demux/decode PASS; NOT playback\n");return 98;
}

static void *worker(void *unused){(void)unused;return (void *)(uintptr_t)decode();}
int _start(void){
 pthread_attr_t attr;pthread_t thread;void *result=0;
 if(pthread_attr_init(&attr))return 10;
 if(pthread_attr_setstacksize(&attr,2u*1024u*1024u)){pthread_attr_destroy(&attr);return 11;}
 int error=pthread_create(&thread,&attr,worker,0);pthread_attr_destroy(&attr);
 if(error)return 12;if(pthread_join(thread,&result))return 13;
 return (int)(uintptr_t)result;
}
