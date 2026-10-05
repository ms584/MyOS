#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
#include "starboard/system.h"
#include "starboard/log.h"
_Static_assert(sizeof(SbMediaSupportType)==4&&sizeof(SbMediaVideoCodec)==4,"pinned media enums");
static int check(void)
{
    const char *mimes[]={0,"","video/webm; codecs=vp9","video/mp4; codecs=avc1","audio/webm; codecs=opus","audio/wav","not-a-mime"};
    const char *keys[]={0,"","com.widevine.alpha","com.widevine.alpha; encryptionscheme=cenc"};
    for(unsigned i=0;i<sizeof(mimes)/sizeof(*mimes);++i)
        for(unsigned j=0;j<sizeof(keys)/sizeof(*keys);++j)
            if(SbMediaCanPlayMimeAndKeySystem(mimes[i],keys[j])!=kSbMediaSupportTypeNotSupported)return 1;
    if(SbMediaCanChangeType(0,0)||SbMediaCanChangeType("audio/webm","video/mp4"))return 2;
    if(SbMediaGetBufferAllocationUnit()!=0||SbMediaGetInitialBufferCapacity()!=0||!SbMediaIsBufferPoolAllocateOnDemand())return 3;
    if(SbMediaGetBufferGarbageCollectionDurationThreshold()!=INT64_C(170000000))return 4;
    int64_t total=SbSystemGetTotalCPUMemory();if(total<=0)return 5;
    uint64_t audio=(uint64_t)total/64,video=(uint64_t)total/16;
    if(audio>4u*1024u*1024u)audio=4u*1024u*1024u;
    if(video>16u*1024u*1024u)video=16u*1024u*1024u;
    if(SbMediaGetAudioBufferBudget()!=(int)audio)return 6;
    if(SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,0,0,0)!=(int)video||
       SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,1920,1080,8)!=(int)video||
       SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,INT_MAX,INT_MAX,INT_MAX)!=(int)video)return 7;
    if(SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,-1,1080,8)||
       SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,1920,-1,8)||
       SbMediaGetVideoBufferBudget(kSbMediaVideoCodecVp9,1920,1080,-1))return 8;
    return 0;
}
static void *worker(void *unused)
{(void)unused;for(unsigned i=0;i<64;++i){int error=check();if(error)return (void *)(uintptr_t)error;}return 0;}
int _start(void)
{
    int error=check();if(error)return error;
    pthread_t threads[4];
    for(unsigned i=0;i<4;++i)if(pthread_create(&threads[i],0,worker,0))return 9;
    for(unsigned i=0;i<4;++i){void *result;if(pthread_join(threads[i],&result)||result)return 10;}
    SbLogRaw("native pinned media policy unsupported codecs/DRM/changeType/direct malloc/170s/RAM-derived budgets/threads PASS\n");
    return 96;
}
