#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
#include "starboard/log.h"
#ifndef DEVICE
#define DEVICE 0
#endif
_Static_assert(sizeof(SbMediaAudioConfiguration)==24&&offsetof(SbMediaAudioConfiguration,latency)==8&&offsetof(SbMediaAudioConfiguration,number_of_channels)==20,"pinned output ABI");
static int check(void)
{
 struct {uint64_t before;SbMediaAudioConfiguration config;uint64_t after;} out;
 out.before=UINT64_C(0x1122334455667788);out.after=UINT64_C(0x8877665544332211);
 unsigned char *p=(unsigned char *)&out.config;
 for(unsigned i=0;i<sizeof(out.config);++i)p[i]=0x5a;
 if(SbMediaGetAudioOutputCount()!=DEVICE)return 1;
 if(SbMediaGetAudioConfiguration(-1,&out.config)||SbMediaGetAudioConfiguration(1,&out.config)||SbMediaGetAudioConfiguration(0,0))return 2;
 for(unsigned i=0;i<sizeof(out.config);++i)if(p[i]!=0x5a)return 3;
 if(SbMediaGetAudioConfiguration(0,&out.config)!=(DEVICE!=0))return 4;
 if(DEVICE){
  if(out.config.connector!=kSbMediaAudioConnectorAnalog||out.config.latency||out.config.coding_type!=kSbMediaAudioCodingTypePcm||out.config.number_of_channels!=1)return 5;
 }else for(unsigned i=0;i<sizeof(out.config);++i)if(p[i]!=0x5a)return 6;
 if(out.before!=UINT64_C(0x1122334455667788)||out.after!=UINT64_C(0x8877665544332211))return 7;
 if(SbMediaCanPlayMimeAndKeySystem("video/mp4; codecs=avc1.42001E","")!=kSbMediaSupportTypeNotSupported)return 8;
 return 0;
}
static void *worker(void *unused){(void)unused;for(unsigned i=0;i<64;++i){int error=check();if(error)return (void *)(uintptr_t)error;}return 0;}
int _start(void)
{
 int error=check();if(error)return error;pthread_t threads[4];
 for(unsigned i=0;i<4;++i)if(pthread_create(&threads[i],0,worker,0))return 9;
 for(unsigned i=0;i<4;++i){void *result;if(pthread_join(threads[i],&result)||result)return 10;}
 SbLogRaw("native pinned audio-output actual SB16/absent device/config/guards/concurrency PASS\n");return 97;
}
