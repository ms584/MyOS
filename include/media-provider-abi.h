#ifndef MYOS_MEDIA_PROVIDER_ABI_H
#define MYOS_MEDIA_PROVIDER_ABI_H
/* Optional paired-image extension. All strong RX exports and a readonly
 * 8-byte version object are validated before any core/media initialization. */
#define MYOS_MEDIA_PROVIDER_VERSION 1
#define MYOS_MEDIA_PROVIDER_EXPORTS(X) \
 X(SbMediaCanPlayMimeAndKeySystem) X(SbMediaCanChangeType) \
 X(SbPlayerCreate) X(SbPlayerDestroy) X(SbPlayerSeek) X(SbPlayerWriteSamples) \
 X(SbPlayerGetMaximumNumberOfSamplesPerWrite) X(SbPlayerWriteEndOfStream) \
 X(SbPlayerSetBounds) X(SbPlayerSetPlaybackRate) X(SbPlayerSetVolume) \
 X(SbPlayerGetInfo) X(SbPlayerGetCurrentFrame) X(SbPlayerGetAudioConfiguration) \
 X(SbPlayerGetPreferredOutputMode) X(SbDecodeTargetRelease) X(SbDecodeTargetGetInfo)
#endif
