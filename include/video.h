#ifndef MYOS_VIDEO_H
#define MYOS_VIDEO_H
#include "os.h"

/* Kernel-mode MYV1 RLE video + unsigned 11025 Hz PCM player. */
bool video_play(const char *path);
/* Fetch a bounded MYV1 file over certificate-verified HTTPS, then play in MyOS. */
bool video_play_net(const char *host,uint16_t port,const char *path);

#endif
