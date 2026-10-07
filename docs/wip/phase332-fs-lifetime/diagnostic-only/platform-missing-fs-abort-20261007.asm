; DIAGNOSTIC ONLY: these are call-abort probes, not functioning APIs.
; They never return to Cobalt or fabricate any success/capability/handle.
default rel
extern MyOSMissingAbort
%macro MISSING 2
section .text
global %1
%1:
    lea rdi,[rel %%name]
    jmp MyOSMissingAbort
section .rodata
%%name: db %2,0
%endmacro
MISSING chmod, "chmod"
MISSING fchmod, "fchmod"
MISSING fchown, "fchown"
MISSING readlink, "readlink"
MISSING symlink, "symlink"
MISSING utimensat, "utimensat"
MISSING inotify_init1, "inotify_init1"
MISSING inotify_add_watch, "inotify_add_watch"
MISSING inotify_rm_watch, "inotify_rm_watch"
MISSING SbPlayerCreate, "SbPlayerCreate"
MISSING SbPlayerDestroy, "SbPlayerDestroy"
MISSING SbPlayerGetAudioConfiguration, "SbPlayerGetAudioConfiguration"
MISSING SbPlayerGetCurrentFrame, "SbPlayerGetCurrentFrame"
MISSING SbPlayerGetInfo, "SbPlayerGetInfo"
MISSING SbPlayerGetMaximumNumberOfSamplesPerWrite, "SbPlayerGetMaximumNumberOfSamplesPerWrite"
MISSING SbPlayerGetPreferredOutputMode, "SbPlayerGetPreferredOutputMode"
MISSING SbPlayerSeek, "SbPlayerSeek"
MISSING SbPlayerSetBounds, "SbPlayerSetBounds"
MISSING SbPlayerSetPlaybackRate, "SbPlayerSetPlaybackRate"
MISSING SbPlayerSetVolume, "SbPlayerSetVolume"
MISSING SbPlayerWriteEndOfStream, "SbPlayerWriteEndOfStream"
MISSING SbPlayerWriteSamples, "SbPlayerWriteSamples"
MISSING SbDecodeTargetGetInfo, "SbDecodeTargetGetInfo"
MISSING SbDecodeTargetRelease, "SbDecodeTargetRelease"
section .note.GNU-stack noalloc noexec nowrite progbits
