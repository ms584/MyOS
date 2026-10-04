/* Typed against the exact pinned Starboard declarations. No host services. */
#include "starboard/system.h"
#include "starboard/log.h"
#include "starboard/time_zone.h"
#include <errno.h>
#include "system-call.h"
extern _Noreturn void abort(void);
static int64_t system_call(uint64_t operation,uint64_t first,uint64_t second)
{
    register uint64_t result __asm__("rax")=MYOS_SYSTEM_CALL;
    __asm__ volatile("int $0x80":"+a"(result):"D"(operation),"S"(first),"d"(second):"memory","cc");
    return (int64_t)result;
}
static void set_failure(int64_t result){if(result<0)errno=(int)-result;}
/* One bounded record is emitted by one non-preemptible kernel call. The raw
 * path has no user lock/allocation/stdio and is usable in a signal handler.
 * Platform log records are limited to64KiB; larger messages keep that prefix. */
void SbLogRaw(const char *message)
{
    if(!message){errno=EFAULT;return;}
    size_t length=0;while(length<SYSTEM_LOG_LIMIT&&message[length])++length;
    set_failure(system_call(SYSTEM_LOG,(uintptr_t)message,length));
}
void SbLog(SbLogPriority priority,const char *message)
{(void)priority;SbLogRaw(message);}
SbSystemError SbSystemGetLastError(void){return errno;}
void SbSystemClearLastError(void){errno=0;}
const char *SbSystemGetLocaleId(void){return "en_US";}
const char *SbTimeZoneGetName(void){return "Etc/UTC";}
int64_t SbSystemGetTotalCPUMemory(void){return system_call(SYSTEM_TOTAL_BYTES,0,0);}
int64_t SbSystemGetUsedCPUMemory(void){return system_call(SYSTEM_OWNED_BYTES,0,0);}
bool SbSystemNetworkIsDisconnected(void){return system_call(SYSTEM_DISCONNECTED,0,0)!=0;}
uint64_t SbSystemGetRandomUInt64(void)
{
    uint64_t value;int64_t rc=system_call(SYSTEM_RANDOM,(uintptr_t)&value,sizeof(value));
    if(rc<0){set_failure(rc);SbLogRaw("MyOS: secure random unavailable; aborting\n");abort();}
    return value;
}
void SbSystemGetRandomData(void *buffer,int size)
{
    if(size<0||(!buffer&&size)){errno=EINVAL;abort();}
    unsigned char *out=buffer;
    while(size){unsigned n=size<256?(unsigned)size:256;
        int64_t rc=system_call(SYSTEM_RANDOM,(uintptr_t)out,n);
        if(rc<0){set_failure(rc);abort();}out+=n;size-=(int)n;}
}
bool SbSystemGetPath(SbSystemPathId id,char *out,int length)
{
    struct system_path_request request={(int32_t)id,length,(uintptr_t)out};
    int64_t rc=system_call(SYSTEM_PATH,(uintptr_t)&request,0);set_failure(rc);return rc==0;
}
static size_t length_of(const char *s){size_t n=0;while(s[n])++n;return n;}
bool SbSystemGetProperty(SbSystemPropertyId id,char *out,int length)
{
    const char *text=0;
    switch(id){
    case kSbSystemPropertyFirmwareVersion:text="0.33";break;
    case kSbSystemPropertyFriendlyName:case kSbSystemPropertyPlatformName:text="MyOS";break;
    case kSbSystemPropertyLimitAdTracking:text="1";break;
    default:errno=(id<0||id>kSbSystemPropertyDeviceType)?EINVAL:ENOTSUP;return false;
    }
    size_t n=length_of(text);
    if(!out||length<=0||(size_t)length<=n){errno=EINVAL;return false;}
    for(size_t i=0;i<=n;++i)out[i]=text[i];return true;
}
int SbSystemGetErrorString(SbSystemError code,char *out,int capacity)
{
    const char *text=0;char unknown[32];
    switch(code){
    case 0:text="Success";break;case EPERM:text="Operation not permitted";break;
    case ENOENT:text="No such file or directory";break;case EIO:text="Input/output error";break;
    case EBADF:text="Bad file descriptor";break;case EAGAIN:text="Resource temporarily unavailable";break;
    case ENOMEM:text="Out of memory";break;case EFAULT:text="Bad address";break;
    case EBUSY:text="Device or resource busy";break;case EEXIST:text="File exists";break;
    case EINVAL:text="Invalid argument";break;case ENOSYS:text="Function not implemented";break;
    case ENOTSUP:text="Operation not supported";break;case ETIMEDOUT:text="Connection timed out";break;
    default:{const char prefix[]="Error ";unsigned at=0;
        for(;prefix[at];++at)unknown[at]=prefix[at];
        uint32_t n=(uint32_t)code;if(code<0){unknown[at++]='-';n=0u-n;}
        char digits[10];unsigned count=0;do{digits[count++]=(char)('0'+n%10);n/=10;}while(n);
        while(count)unknown[at++]=digits[--count];unknown[at]=0;text=unknown;break;}
    }
    size_t length=length_of(text);
    if(out&&capacity>0){size_t n=(size_t)(capacity-1);if(n>length)n=length;
        for(size_t i=0;i<n;++i)out[i]=text[i];out[n]=0;}
    return (int)length;
}
/* The target has no unwind metadata service. Return the real caller PC, never
 * invented frame chains. This gives one frame, within the API's 'up to' bound. */
int SbSystemGetStack(void **out,int capacity)
{if(capacity<=0||!out)return 0;out[0]=__builtin_return_address(0);return 1;}
bool SbSystemSymbolize(const void *address,char *out,int capacity)
{(void)address;(void)out;(void)capacity;return false;}
bool SbSystemHasCapability(SbSystemCapabilityId id){(void)id;return false;}
int64_t SbSystemGetUsedGPUMemory(void){errno=ENOTSUP;return -1;}
int64_t SbSystemGetTotalGPUMemory(void){errno=ENOTSUP;return -1;}
const void *SbSystemGetExtension(const char *name){(void)name;return 0;}
bool SbSystemIsDebuggerAttached(void){return false;}
void SbSystemBreakIntoDebugger(void){abort();}
void SbSystemHideSplashScreen(void){} /* MyOS has no application splash overlay. */
bool SbSystemRaisePlatformError(SbSystemPlatformErrorType type,SbSystemPlatformErrorCallback callback,void *context)
{(void)type;(void)callback;(void)context;return false;}
bool SbSystemSignWithCertificationSecretKey(const uint8_t *message,size_t size,uint8_t *digest,size_t capacity)
{(void)message;(void)size;(void)digest;(void)capacity;return false;}
_Static_assert(sizeof(SbSystemError)==4&&sizeof(SbSystemPathId)==4&&sizeof(SbSystemPropertyId)==4&&
               sizeof(SbLogPriority)==4&&sizeof(bool)==1&&sizeof(void *)==8,"pinned scalar ABI");
