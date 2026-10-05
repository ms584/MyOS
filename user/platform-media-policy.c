/* Phase33.2 policy only. No demux, decoder, player or protected media support. */
#include <stdint.h>
#include "starboard/configuration.h"
#include "starboard/media.h"
#include "starboard/system.h"
_Static_assert(sizeof(int)==4&&sizeof(int64_t)==8,"pinned media LP64 ABI");

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
