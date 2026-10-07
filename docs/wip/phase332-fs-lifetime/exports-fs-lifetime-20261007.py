#!/usr/bin/env python3
"""Generate fixed-address Ring 3 platform runtime exports from nm output."""
import subprocess
import hashlib
import json
import os
import sys
from pathlib import Path
elf=Path(sys.argv[1]);output=Path(sys.argv[2])
wanted={'__errno_location','access','abort','signal','sigaction','raise','pause','MyOSSignalReturn','aligned_alloc','calloc','clock_gettime','clock_nanosleep','close','closedir',
        'SbMediaCanChangeType','SbMediaCanPlayMimeAndKeySystem','SbMediaGetAudioBufferBudget',
        'SbMediaGetBufferAllocationUnit','SbMediaGetBufferGarbageCollectionDurationThreshold',
        'SbMediaGetInitialBufferCapacity','SbMediaGetVideoBufferBudget','SbMediaIsBufferPoolAllocateOnDemand',
        'SbAudioSinkCreate','SbAudioSinkDestroy','SbAudioSinkGetMinBufferSizeInFrames',
        'SbAudioSinkGetNearestSupportedSampleFrequency','SbAudioSinkIsAudioSampleTypeSupported',
        'SbAudioSinkGetMaxChannels','SbAudioSinkIsValid','SbAudioSinkIsAudioFrameStorageTypeSupported',
        'getaddrinfo','freeaddrinfo','gai_strerror','sendmsg','recvmsg','sendto','recvmmsg',
        'getifaddrs','freeifaddrs','if_indextoname',
        'SbLog','SbLogRaw','SbSystemBreakIntoDebugger','SbSystemGetErrorString','SbSystemGetExtension',
        'SbSystemGetLastError','SbSystemGetLocaleId','SbSystemGetPath','SbSystemGetProperty',
        'SbSystemGetRandomUInt64','SbSystemGetRandomData','SbSystemGetStack','SbSystemGetTotalCPUMemory',
        'SbSystemGetUsedCPUMemory','SbSystemGetUsedGPUMemory','SbSystemGetTotalGPUMemory',
        'SbSystemHasCapability','SbSystemHideSplashScreen','SbSystemIsDebuggerAttached',
        'SbSystemNetworkIsDisconnected','SbSystemRaisePlatformError',
        'SbSystemSignWithCertificationSecretKey','SbSystemSymbolize','SbTimeZoneGetName','SbSystemClearLastError',
        'SbDrmCloseSession','SbDrmCreateSystem','SbDrmDestroySystem',
        'SbDrmGenerateSessionUpdateRequest','SbDrmGetMetrics','SbDrmIsServerCertificateUpdatable',
        'SbDrmUpdateServerCertificate','SbDrmUpdateSession','SbMicrophoneCreate',
        'SbMicrophoneDestroy','SbMicrophoneGetAvailable','SbMicrophoneOpen','SbMicrophoneRead',
        'SbStorageCloseRecord','SbStorageDeleteRecord','SbStorageGetRecordSize',
        'SbStorageOpenRecord','SbStorageReadRecord','SbStorageWriteRecord',
        'SbSpeechSynthesisCancel','SbSpeechSynthesisIsSupported','SbSpeechSynthesisSpeak',
        'SbWindowCreate','SbWindowDestroy','SbWindowGetSize','SbWindowGetPlatformHandle',
        'SbWindowSetDefaultOptions','SbWindowGetDiagonalSizeInInches',
        'MyOSUiQuery','MyOSUiPresent','MyOSUiPoll',
        'MyOSUiWindow','MyOSUiSetVisible','SbRunStarboardMain',
        'MyOSStarboardRun','MyOSStarboardValidate',
        'MyOSFinalizeImages',
        '__tls_get_addr','MyOSTlsDesc',
        'SbEventSchedule','SbEventCancel','SbSystemRequestStop','SbSystemRequestConceal',
        'SbSystemRequestReveal','SbSystemRequestBlur','SbSystemRequestFocus','SbSystemSupportsResume',
        'accept','bind','listen','connect','dup','epoll_create1','epoll_ctl','epoll_wait','eventfd','fcntl','fdatasync','fdopendir','free',
        'fstat','fstatat','fsync','ftruncate','getcwd','ioctl_FIONREAD','isatty','lseek','madvise','malloc','mkdir','mkdtemp','mkstemp',
        'getauxval','geteuid','getpeername','getpid','getrlimit','getsockname','getsockopt','gettid','getuid',
        'kSbFileMaxName','kSbFileMaxPath','kSbFileSepString','kSbMaxThreads',
        'malloc_usable_size','mincore','mmap','mprotect','msync','munmap','prctl',
        'openat','opendir','pathconf','pipe','pipe2','poll','posix_memalign','pread','pwrite','read','readdir','readdir_r',
        'readv','realloc','realpath','recv','rename','send','setsockopt','shutdown','socket','socketpair','unlinkat','write','writev',
        'sched_getaffinity','sched_setscheduler','sched_yield','setpriority','sysconf','uname',
        'pthread_attr_destroy','pthread_attr_getstack','pthread_attr_getstacksize',
        'pthread_attr_init','pthread_attr_setdetachstate','pthread_attr_setstacksize',
        'pthread_cond_broadcast','pthread_cond_destroy','pthread_cond_init',
        'pthread_cond_signal','pthread_cond_timedwait','pthread_cond_wait',
        'pthread_condattr_destroy','pthread_condattr_init','pthread_condattr_setclock',
        'pthread_create','pthread_detach','pthread_equal','pthread_exit','pthread_mutex_timedlock','pthread_getattr_np','pthread_getname_np',
        'pthread_getschedparam','pthread_getspecific','pthread_join','pthread_key_create',
        'pthread_key_delete','pthread_kill','pthread_mutex_destroy','pthread_mutex_init',
        'pthread_mutex_lock','pthread_mutex_trylock','pthread_mutex_unlock',
        'pthread_mutexattr_destroy','pthread_mutexattr_init','pthread_mutexattr_settype',
        'pthread_once','pthread_rwlock_destroy','pthread_rwlock_init',
        'pthread_rwlock_rdlock','pthread_rwlock_unlock','pthread_rwlock_wrlock',
        'pthread_self','pthread_setname_np','pthread_setspecific','pthread_sigmask',
        'sem_destroy','sem_init','sem_post','sem_timedwait','sem_wait'}
wanted.update({'statvfs','SbMediaGetAudioOutputCount','SbMediaGetAudioConfiguration'})
symbols={}
for line in subprocess.check_output(['nm','-g','--defined-only',str(elf)],text=True).splitlines():
    fields=line.split()
    if len(fields)==3 and fields[2] in wanted:symbols[fields[2]]=int(fields[0],16)
missing=wanted-symbols.keys()
if missing:raise SystemExit('missing runtime exports: '+', '.join(sorted(missing)))
lines=['/* Generated by tools/make_runtime_exports.py; do not edit. */']
raw_size=elf.with_suffix('.bin').stat().st_size
if not 0<raw_size<=65536:raise SystemExit('invalid raw runtime size')
lines.append(f'#define PLATFORM_RUNTIME_BYTES {raw_size}u')
packed=elf.with_suffix('.mrt').read_bytes()
lines.append(f'#define PLATFORM_RUNTIME_PACKED_BYTES {len(packed)}u')
digest=','.join(f'0x{byte:02x}' for byte in hashlib.sha256(packed).digest())
lines.append('#define PLATFORM_RUNTIME_PACKED_SHA256 {'+digest+'}')
external=int(os.environ.get('PROCESS_RUNTIME_EXTERNAL','1'))
if external not in (0,1):raise SystemExit('invalid runtime profile')
elf.with_suffix('.manifest.json').write_text(json.dumps({
    'format':'MRT1','external':bool(external),'raw_bytes':raw_size,
    'packed_bytes':len(packed),'packed_sha256':hashlib.sha256(packed).hexdigest()},
    indent=2)+'\n')
ordered=sorted(symbols)
for name in ordered:lines.append(f'#define PLATFORM_{name.upper()} 0x{symbols[name]:x}ull')
lines.append('#define PLATFORM_EXPORTS(X) \\')
for index,name in enumerate(ordered):
    tail=' \\' if index+1<len(ordered) else ''
    lines.append(f' X({name}, PLATFORM_{name.upper()}){tail}')
output.write_text('\n'.join(lines)+'\n')
