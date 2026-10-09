#ifndef MYOS_AUDIO_H
#define MYOS_AUDIO_H
#include "os.h"

/* Phase 30: QEMU SB16, unsigned 8-bit mono PCM at 11025 Hz. */
bool audio_init(void);
bool audio_ready(void);
struct audio_stream_status {
    uint64_t consumed;
    uint32_t queued,dma_frames,capacity,sample_rate,channels,sample_bits;
};
int audio_stream_acquire(uint64_t owner);
int audio_stream_queue(uint64_t owner,const uint8_t *pcm,size_t count);
int audio_stream_status(uint64_t owner,struct audio_stream_status *status);
void audio_stream_release(uint64_t owner);
int audio_stream_flush(uint64_t owner);
int audio_stream_start(uint64_t owner);
int audio_stream_configure(uint64_t owner,unsigned half_frames);
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
