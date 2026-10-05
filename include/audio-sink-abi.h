#ifndef MYOS_AUDIO_SINK_ABI_H
#define MYOS_AUDIO_SINK_ABI_H
#include <stdbool.h>
/* Typed target-only subset of pinned Cobalt3befd332 audio_sink.h/media.h,
 * SB_API_VERSION18. Native x64 enum/int widths and pointer/callback ABI. */
typedef enum {kSbMediaAudioSampleTypeInt16Deprecated,kSbMediaAudioSampleTypeFloat32} SbMediaAudioSampleType;
typedef enum {kSbMediaAudioFrameStorageTypeInterleaved,kSbMediaAudioFrameStorageTypePlanar} SbMediaAudioFrameStorageType;
typedef void **SbAudioSinkFrameBuffers;
typedef struct SbAudioSinkPrivate *SbAudioSink;
typedef void (*SbAudioSinkUpdateSourceStatusFunc)(int *,int *,bool *,bool *,void *);
typedef void (*SbAudioSinkConsumeFramesFunc)(int,void *);
#define kSbAudioSinkInvalid ((SbAudioSink)0)
SbAudioSink SbAudioSinkCreate(int,int,SbMediaAudioSampleType,SbMediaAudioFrameStorageType,
 SbAudioSinkFrameBuffers,int,SbAudioSinkUpdateSourceStatusFunc,SbAudioSinkConsumeFramesFunc,void *);
void SbAudioSinkDestroy(SbAudioSink);
bool SbAudioSinkIsValid(SbAudioSink);
int SbAudioSinkGetMaxChannels(void);
int SbAudioSinkGetNearestSupportedSampleFrequency(int);
bool SbAudioSinkIsAudioSampleTypeSupported(SbMediaAudioSampleType);
bool SbAudioSinkIsAudioFrameStorageTypeSupported(SbMediaAudioFrameStorageType);
int SbAudioSinkGetMinBufferSizeInFrames(int,SbMediaAudioSampleType,int);
_Static_assert(sizeof(SbMediaAudioSampleType)==4&&sizeof(SbMediaAudioFrameStorageType)==4&&sizeof(SbAudioSink)==8,"pinned audio ABI");
#endif
