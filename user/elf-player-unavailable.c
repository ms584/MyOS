#include <stdbool.h>
#include "starboard/player.h"
extern void SbLogRaw(const char *);
static unsigned callbacks;
static void deallocate(SbPlayer p,void *c,const void *b){(void)p;(void)c;(void)b;++callbacks;}
static void decoder(SbPlayer p,void *c,SbMediaType t,SbPlayerDecoderState s,int n)
{(void)p;(void)c;(void)t;(void)s;(void)n;++callbacks;}
static void status(SbPlayer p,void *c,SbPlayerState s,int n)
{(void)p;(void)c;(void)s;(void)n;++callbacks;}
static void error(SbPlayer p,void *c,SbPlayerError e,const char *m)
{(void)p;(void)c;(void)e;(void)m;++callbacks;}
#define CHECK(x) do{if(!(x)){SbLogRaw("Player unavailable FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 SbPlayerCreationParam param={0};param.output_mode=kSbPlayerOutputModeInvalid;
 for(unsigned i=0;i<64;++i){
  CHECK(SbPlayerCreate(0,&param,deallocate,decoder,status,error,&callbacks,0)==kSbPlayerInvalid);
  CHECK(SbPlayerGetPreferredOutputMode(&param)==kSbPlayerOutputModeInvalid);
 }
 CHECK(SbPlayerCreate(0,0,0,0,0,0,0,0)==kSbPlayerInvalid&&!callbacks);
 struct {uint64_t before;SbPlayerInfo value;uint64_t after;} info={.before=42,.after=99};
 unsigned char *bytes=(void *)&info.value;for(size_t i=0;i<sizeof(info.value);++i)bytes[i]=0xa5;
 SbPlayerGetInfo(0,&info.value);
 for(size_t i=0;i<sizeof(info.value);++i)CHECK(bytes[i]==0xa5);
 CHECK(info.before==42&&info.after==99);
 SbMediaAudioConfiguration audio={0};audio.number_of_channels=77;
 CHECK(!SbPlayerGetAudioConfiguration(0,0,&audio)&&audio.number_of_channels==77);
 CHECK(SbPlayerGetCurrentFrame(0)==kSbDecodeTargetInvalid);
 CHECK(!SbPlayerSetPlaybackRate(0,1.0));
 CHECK(SbPlayerGetMaximumNumberOfSamplesPerWrite(0,kSbMediaTypeAudio)==0);
 SbDecodeTargetInfo target={0};
 CHECK(!SbDecodeTargetGetInfo(0,&target));
 SbDecodeTargetRelease(0);SbPlayerDestroy(0);SbPlayerSeek(0,0,1);
 SbPlayerSetBounds(0,0,0,0,800,600);SbPlayerSetVolume(0,1.0);
 SbPlayerWriteSamples(0,kSbMediaTypeAudio,0,0);SbPlayerWriteEndOfStream(0,kSbMediaTypeAudio);
 CHECK(!callbacks);
 SbLogRaw("native unavailable player/decode15 binding/invalid handles/no outputs/no callbacks/no allocation PASS\n");
 return 92;
}
