#include "starboard/system.h"
#include "starboard/log.h"
#include "starboard/time_zone.h"
#include "system-call.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
extern void *mmap(void *,size_t,int,int,int,long);
extern int munmap(void *,size_t);
#ifndef MODE
#define MODE 0
#endif
static int64_t call(uint64_t op,uint64_t a,uint64_t b)
{register uint64_t r __asm__("rax")=MYOS_SYSTEM_CALL;
 __asm__ volatile("int $0x80":"+a"(r):"D"(op),"S"(a),"d"(b):"memory","cc");return (int64_t)r;}
#define CHECK(x) do{if(!(x)){SbLogRaw("system failed: " #x "\n");__builtin_trap();}}while(0)
static volatile unsigned handled;
static void handler(int sig){CHECK(sig==SIGUSR1);SbLogRaw("system signal raw PASS\n");++handled;}
static void unexpected(SbSystemPlatformErrorResponse r,void *p){(void)r;(void)p;__builtin_trap();}
static void no_entropy(int sig){CHECK(sig==SIGABRT);SbLogRaw("secure random fail-closed PASS\n");
 register uint64_t r __asm__("rax")=3;
 __asm__ volatile("int $0x80":"+a"(r):"D"(84):"memory","cc");__builtin_unreachable();}
static void *worker(void *raw)
{
 unsigned id=(unsigned)(uintptr_t)raw;char record[2050];
 memset(record,'A'+id,2048);record[2048]='\n';record[2049]=0;
 errno=200+(int)id;
 for(unsigned i=0;i<3;++i){SbLog(kSbLogPriorityFatal,record);
   CHECK(SbSystemGetLastError()==200+(int)id);
   CHECK(!strcmp(SbSystemGetLocaleId(),"en_US"));
   char path[64];CHECK(SbSystemGetPath(kSbSystemPathCacheDirectory,path,sizeof(path)));
   CHECK(!strcmp(path,"/home/cobalt/cache"));
   struct timespec pause={0,1000000};CHECK(!clock_nanosleep(CLOCK_MONOTONIC,0,&pause,0));}
 SbSystemClearLastError();CHECK(!errno);return raw;
}
int _start(void)
{
 if(MODE==1){CHECK(!SbSystemNetworkIsDisconnected());SbLogRaw("network connected PASS\n");return 85;}
 if(MODE==2){CHECK(SbSystemNetworkIsDisconnected());SbLogRaw("network disconnected PASS\n");return 86;}
 if(MODE==3){struct sigaction action={0};action.sa_handler=no_entropy;
   CHECK(!sigaction(SIGABRT,&action,0));(void)SbSystemGetRandomUInt64();__builtin_trap();}
 if(MODE==4){struct sigaction action={0};action.sa_handler=no_entropy;
   CHECK(!sigaction(SIGABRT,&action,0));SbSystemBreakIntoDebugger();}
 CHECK(!strcmp(SbSystemGetLocaleId(),"en_US")&&!strcmp(SbTimeZoneGetName(),"Etc/UTC"));
 char out[96];memset(out,'Z',sizeof(out));
 CHECK(SbSystemGetErrorString(EINVAL,0,0)==16);
 CHECK(SbSystemGetErrorString(EINVAL,out,4)==16&&!strcmp(out,"Inv")&&out[4]=='Z');
 CHECK(SbSystemGetErrorString(-2147483647-1,out,sizeof(out))==17&&!strcmp(out,"Error -2147483648"));
 out[0]='Z';CHECK(SbSystemGetErrorString(0,out,0)==7&&out[0]=='Z');
 CHECK(SbSystemGetProperty(kSbSystemPropertyPlatformName,out,sizeof(out))&&!strcmp(out,"MyOS"));
 memset(out,'Z',sizeof(out));
 CHECK(!SbSystemGetProperty(kSbSystemPropertyPlatformName,out,4)&&out[0]=='Z');
 CHECK(!SbSystemGetProperty(kSbSystemPropertyCertificationScope,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemGetProperty((SbSystemPropertyId)999,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemGetProperty(kSbSystemPropertyPlatformName,0,100));
 CHECK(!SbSystemHasCapability(kSbSystemCapabilityCanQueryGPUMemoryStats));
 CHECK(!SbSystemHasCapability((SbSystemCapabilityId)999));
 CHECK(SbSystemGetUsedGPUMemory()==-1&&SbSystemGetTotalGPUMemory()==-1);
 CHECK(!SbSystemIsDebuggerAttached()&&!SbSystemGetExtension("nonexistent"));
 CHECK(!SbSystemSymbolize((void *)_start,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemSignWithCertificationSecretKey((void *)"x",1,(void *)out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemRaisePlatformError(kSbSystemPlatformErrorTypeConnectionError,unexpected,0));
 SbSystemHideSplashScreen();SbSystemHideSplashScreen();
 void *stack[3]={(void *)1,(void *)2,(void *)3};
 CHECK(!SbSystemGetStack(stack,0)&&stack[0]==(void *)1);
 CHECK(SbSystemGetStack(stack,3)==1&&(uintptr_t)stack[0]>0x8000000000ULL&&stack[1]==(void *)2);
 CHECK(SbSystemGetPath(kSbSystemPathContentDirectory,out,sizeof(out))&&!strcmp(out,"/assets"));
 const int ids[]={1,2,5,8};const char *paths[]={"/home/cobalt/cache","/home/cobalt/debug","/home/cobalt/tmp","/home/cobalt"};
 for(unsigned i=0;i<4;++i){CHECK(SbSystemGetPath((SbSystemPathId)ids[i],out,sizeof(out)));CHECK(!strcmp(out,paths[i]));}
 memset(out,'Z',sizeof(out));
 CHECK(!SbSystemGetPath(kSbSystemPathContentDirectory,out,7)&&out[0]=='Z');
 CHECK(!SbSystemGetPath(kSbSystemPathFontDirectory,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemGetPath((SbSystemPathId)999,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemGetPath(kSbSystemPathExecutableFile,out,sizeof(out))&&out[0]=='Z');
 CHECK(!SbSystemGetPath(kSbSystemPathContentDirectory,0,100));
 struct system_path_request req={0,96,(uintptr_t)"read-only"};
 CHECK(call(SYSTEM_PATH,(uintptr_t)&req,0)==-14);
 CHECK(call(SYSTEM_PATH,1,0)==-14&&call(SYSTEM_PATH,(uintptr_t)&req,1)==-22);
 CHECK(call(999,0,0)==-22&&call(SYSTEM_TOTAL_BYTES,1,0)==-22);
 CHECK(call(SYSTEM_LOG,1,10)==-14&&call(SYSTEM_LOG,0,SYSTEM_LOG_LIMIT+1)==-22);
 CHECK(call(SYSTEM_RANDOM,(uintptr_t)out,257)==-22);
 CHECK(call(SYSTEM_RANDOM,(uintptr_t)"read-only",8)==-14);
 CHECK(call(SYSTEM_RANDOM,0,0)==0);
 int64_t total=SbSystemGetTotalCPUMemory(),used=SbSystemGetUsedCPUMemory();
 CHECK(total>0&&used>0&&used<=total);
 void *page=mmap(0,65536,3,0x22,-1,0);CHECK(page!=(void *)-1);
 CHECK(SbSystemGetUsedCPUMemory()>=used+65536);CHECK(!munmap(page,65536));
 int64_t warm=SbSystemGetUsedCPUMemory();page=mmap(0,65536,3,0x22,-1,0);CHECK(page!=(void *)-1);
 CHECK(!munmap(page,65536)&&SbSystemGetUsedCPUMemory()==warm);
 CHECK(SbSystemNetworkIsDisconnected());
 uint64_t random[16];for(unsigned i=0;i<16;++i)random[i]=SbSystemGetRandomUInt64();
 unsigned distinct=0;for(unsigned i=1;i<16;++i)distinct+=random[i]!=random[0];CHECK(distinct>0);
 unsigned char bytes[513];SbSystemGetRandomData(bytes,sizeof(bytes));
 struct sigaction action={0};action.sa_handler=handler;CHECK(!sigaction(SIGUSR1,&action,0));
 CHECK(!raise(SIGUSR1)&&handled==1);
 pthread_t threads[8];for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)i));
 for(unsigned i=0;i<8;++i){void *result;CHECK(!pthread_join(threads[i],&result)&&result==(void *)(uintptr_t)i);}
 SbLogRaw("typed system/logging/memory/paths/random PASS\n");return 83;
}
