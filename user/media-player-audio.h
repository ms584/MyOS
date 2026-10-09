#ifndef MYOS_MEDIA_PLAYER_AUDIO_H
#define MYOS_MEDIA_PLAYER_AUDIO_H
/* Uses the full pinned media enums already included by player.h. */
typedef struct SbAudioSinkPrivate *SbAudioSink;
typedef void **SbAudioSinkFrameBuffers;
typedef void (*SbAudioSinkUpdateSourceStatusFunc)(int *,int *,bool *,bool *,void *);
typedef void (*SbAudioSinkConsumeFramesFunc)(int,void *);
SbAudioSink SbAudioSinkCreate(int,int,SbMediaAudioSampleType,SbMediaAudioFrameStorageType,
 SbAudioSinkFrameBuffers,int,SbAudioSinkUpdateSourceStatusFunc,SbAudioSinkConsumeFramesFunc,void *);
void SbAudioSinkDestroy(SbAudioSink);
#endif
