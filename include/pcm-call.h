#ifndef MYOS_PCM_CALL_H
#define MYOS_PCM_CALL_H
#include <stdint.h>
/* PRIVATE native PCM bridge draft; not exported or installed in production. */
#define SYSTEM_AUDIO 8u
enum {PCM_QUERY,PCM_ACQUIRE,PCM_QUEUE,PCM_STATUS,PCM_FLUSH,PCM_RELEASE,PCM_START};
struct pcm_request {uint32_t action,reserved;uint64_t buffer,length,result;};
struct pcm_status {uint64_t consumed;uint32_t queued,dma_frames,capacity,sample_rate,channels,sample_bits;};
_Static_assert(sizeof(struct pcm_request)==32&&sizeof(struct pcm_status)==32,"PCM LP64 syscall ABI");
#endif
