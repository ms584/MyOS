/* Bounded WebM subset reader for native asset/stream checkpoints.
 * Production Cobalt uses its own pinned demuxer; no claim of full Matroska. */
#ifndef MYOS_MEDIA_WEBM_H
#define MYOS_MEDIA_WEBM_H
#include <stddef.h>
#include <stdint.h>
#define MYOS_WEBM_PACKET_CAP (256u*1024u)
typedef int (*myos_webm_read)(void *,void *,size_t);
typedef int (*myos_webm_packet)(void *,int,const unsigned char *,size_t,int64_t,int);
struct myos_webm_source {myos_webm_read read;void *context;uint64_t length;};
/* Callback: track kind 1=VP8, 2=Opus; timestamp in microseconds, key-frame flag.
 * Borrowed packet memory expires on callback return. Return nonzero to stop.
 * Rejects lacing/encryption/unknown cluster sizes rather than invent samples.
 * 0=success; -1=invalid/truncated/unsupported; -2=allocation; -3=callback. */
int myos_webm_demux(const struct myos_webm_source *,myos_webm_packet,void *);
#endif
