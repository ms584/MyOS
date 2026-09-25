#ifndef MYOS_AUDIO_H
#define MYOS_AUDIO_H
#include "os.h"

/* Phase 30: QEMU SB16, unsigned 8-bit mono PCM at 11025 Hz. */
bool audio_init(void);
bool audio_ready(void);
void audio_irq(void);
bool audio_queue(const uint8_t *pcm,size_t count);
void audio_stop(void);
void audio_set_volume(unsigned percent);
unsigned audio_volume(void);
void audio_set_mute(bool on);
bool audio_muted(void);
uint64_t audio_underruns(void);
uint64_t audio_interrupts(void);
size_t audio_queued(void);
bool audio_tone(unsigned hz,unsigned milliseconds);
bool audio_play_wav(const char *path);
void audio_print_status(void);
#endif
