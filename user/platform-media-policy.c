/* Phase33.2 policy only. No demux, decoder, player or protected media support. */
#include <stdint.h>
#include <stddef.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
#include "starboard/system.h"
#include "pcm-call.h"
#include "system-call.h"
_Static_assert(sizeof(int)==4&&sizeof(int64_t)==8,"pinned media LP64 ABI");
_Static_assert(sizeof(SbMediaAudioConfiguration)==24&&offsetof(SbMediaAudioConfiguration,latency)==8&&offsetof(SbMediaAudioConfiguration,number_of_channels)==20,"pinned audio configuration ABI");

static int query_audio(struct pcm_status *status)
{
    struct pcm_request request={PCM_QUERY,0,0,0,(uintptr_t)status};
    register uint64_t result __asm__("rax")=MYOS_SYSTEM_CALL;
    __asm__ volatile("int $0x80":"+a"(result):"D"((uint64_t)SYSTEM_AUDIO),"S"((uintptr_t)&request),"d"((uint64_t)0):"memory","cc");
    return (int64_t)result<0?-1:0;
}
int SbMediaGetAudioOutputCount(void)
{
    struct pcm_status status;
    return !query_audio(&status)&&status.channels&&status.sample_rate?1:0;
}
bool SbMediaGetAudioConfiguration(int index,SbMediaAudioConfiguration *out)
{
    if(index!=0||!out)return false;struct pcm_status status;
    if(query_audio(&status)||!status.channels||!status.sample_rate)return false;
    /* Actual native SB16 PCM bridge, currently mono. Do not advertise stereo
     * or a measured latency that the driver does not provide. */
    for(size_t i=0;i<sizeof(*out);++i)((unsigned char *)out)[i]=0;
    out->connector=kSbMediaAudioConnectorAnalog;
    out->coding_type=kSbMediaAudioCodingTypePcm;
    out->number_of_channels=(int)status.channels;return true;
}

SbMediaSupportType SbMediaCanPlayMimeAndKeySystem(const char *mime,const char *key_system)
{
    (void)mime;(void)key_system;
    /* Native PCM plumbing is not a MIME/container/codec playback pipeline. */
    return kSbMediaSupportTypeNotSupported;
}
bool SbMediaCanChangeType(const char *current_mime,const char *new_mime)
{(void)current_mime;(void)new_mime;return false;}

/* Limits are policy for source-buffer garbage collection, not reservations or
 * promises that decoding works. Derive them from actual native memory query;
 * no writable global, allocation, host query or invented available memory. */
static int budget(unsigned divisor,unsigned ceiling)
{
    int64_t total=SbSystemGetTotalCPUMemory();
    if(total<=0)return 0;
    uint64_t limit=(uint64_t)total/divisor;
    if(limit>ceiling)limit=ceiling;
    return (int)limit;
}
int SbMediaGetAudioBufferBudget(void){return budget(64,4u*1024u*1024u);}
int SbMediaGetVideoBufferBudget(SbMediaVideoCodec codec,int width,int height,int bits_per_pixel)
{
    (void)codec;
    if(width<0||height<0||bits_per_pixel<0)return 0;
    /* Zero is the documented unknown dimension/bpp, not a negative error.
     * One conservative global cap applies even to unknown/extreme sizes. */
    return budget(16,16u*1024u*1024u);
}
int SbMediaGetBufferAllocationUnit(void){return 0;}
int SbMediaGetInitialBufferCapacity(void){return 0;}
bool SbMediaIsBufferPoolAllocateOnDemand(void){return true;}
int64_t SbMediaGetBufferGarbageCollectionDurationThreshold(void)
{return INT64_C(170000000);}
