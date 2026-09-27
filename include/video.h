#ifndef MYOS_VIDEO_H
#define MYOS_VIDEO_H
#include "os.h"

/* Kernel-mode MYV1 RLE video + unsigned 11025 Hz PCM player. */
bool video_play(const char *path);

#endif
