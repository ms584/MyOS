#include <stddef.h>
#include <stdint.h>

struct musl_rlimit { uint64_t current,maximum; };
struct musl_cpu_set { uint64_t bits[16]; };
struct musl_utsname { char sysname[257],nodename[257],release[257],version[257],machine[257],domainname[257]; };

extern int *__errno_location(void);
extern int32_t getpid(void),gettid(void);
extern uint32_t getuid(void),geteuid(void);
extern long sysconf(int);
extern unsigned long getauxval(unsigned long);
extern int getrlimit(int,struct musl_rlimit *),sched_getaffinity(int32_t,size_t,struct musl_cpu_set *);
extern int sched_setscheduler(int32_t,int,const int *),sched_yield(void),setpriority(int,uint32_t,int);
extern int prctl(int,...),uname(struct musl_utsname *);
extern int pthread_create(uint64_t *,const void *,void *(*)(void *),void *),pthread_join(uint64_t,void **);
extern int64_t write(int,const void *,size_t);
extern const int32_t kSbFileMaxName;
extern const uint32_t kSbFileMaxPath,kSbMaxThreads;
extern const char *const kSbFileSepString;
extern void *SbDrmCreateSystem(const char *,void *,void *,void *,void *,void *,void *);
extern const void *SbDrmGetMetrics(void *,int *);
extern int SbDrmIsServerCertificateUpdatable(void *);
extern void SbDrmCloseSession(void *,const void *,int),SbDrmDestroySystem(void *);
extern void SbDrmGenerateSessionUpdateRequest(void *,int,const char *,const void *,int);
extern void SbDrmUpdateSession(void *,int,const void *,int,const void *,int);
extern void SbDrmUpdateServerCertificate(void *,int,const void *,int);
extern int SbMicrophoneGetAvailable(void *,int),SbMicrophoneOpen(void *),SbMicrophoneRead(void *,void *,int);
extern void *SbMicrophoneCreate(int,int,int);
extern void SbMicrophoneDestroy(void *),SbSpeechSynthesisSpeak(const char *),SbSpeechSynthesisCancel(void);
extern int SbSpeechSynthesisIsSupported(void);

static int32_t worker_tid;
static void *worker(void *unused){(void)unused;worker_tid=gettid();return 0;}
static int equal(const char *first,const char *second)
{
    while(*first&&*first==*second){++first;++second;}return *first==*second;
}
static void print(const char *text)
{
    size_t length=0;while(text[length])++length;(void)write(1,text,length);
}
uint64_t _start(void)
{
    int32_t pid=getpid(),tid=gettid();if(pid<=0||tid!=pid||getuid()!=0||geteuid()!=0)return 1;
    uint64_t thread;if(pthread_create(&thread,0,worker,0)||pthread_join(thread,0))return 2;
    if(worker_tid<=0||worker_tid==pid)return 3;

    if(sysconf(30)!=4096||sysconf(2)!=100||sysconf(83)!=1||sysconf(84)!=1||
       sysconf(74)!=32||sysconf(76)!=8)return 4;
    *__errno_location()=0;if(sysconf(999)!=-1||*__errno_location()!=22)return 5;
    if(getauxval(6)!=4096||getauxval(17)!=100||getauxval(11)!=0)return 6;
    *__errno_location()=0;if(getauxval(999)!=0||*__errno_location()!=2)return 7;

    struct musl_rlimit limit;
    if(getrlimit(7,&limit)||limit.current!=40||limit.maximum!=40)return 8;
    if(getrlimit(6,&limit)||limit.current!=8||limit.maximum!=8)return 9;
    if(getrlimit(99,&limit)!=-1||*__errno_location()!=22)return 10;

    struct musl_cpu_set mask;
    if(sched_getaffinity(0,sizeof(mask),&mask)||mask.bits[0]!=1)return 11;
    for(unsigned i=1;i<16;++i)if(mask.bits[i])return 12;
    if(sched_getaffinity(0,8,&mask)!=-1||*__errno_location()!=22)return 13;
    int parameter=0;if(sched_setscheduler(0,0,&parameter)||sched_yield())return 14;
    parameter=1;if(sched_setscheduler(0,0,&parameter)!=-1||*__errno_location()!=22)return 15;
    if(setpriority(0,0,0)||setpriority(0,0,1)!=-1||*__errno_location()!=1)return 16;

    char name[16];if(prctl(15,(void *)"system")||prctl(16,name)||!equal(name,"system"))return 17;
    if(prctl(15,(void *)"abcdefghijklmnopq")||prctl(16,name)||!equal(name,"abcdefghijklmno"))return 24;
    if(prctl(12345,(void *)0)!=-1||*__errno_location()!=22)return 18;
    struct musl_utsname identity;if(uname(&identity)||!equal(identity.sysname,"MyOS")||
       !equal(identity.machine,"x86_64")||!equal(identity.nodename,"myos"))return 19;
    if(kSbFileMaxName!=31||kSbFileMaxPath!=127||kSbMaxThreads!=8||
       !kSbFileSepString||kSbFileSepString[0]!='/'||kSbFileSepString[1])return 20;
    int metrics=7;if(SbDrmCreateSystem("com.example",0,(void *)1,(void *)1,(void *)1,(void *)1,(void *)1)||
       SbDrmIsServerCertificateUpdatable(0)||SbDrmGetMetrics(0,&metrics)||metrics)return 21;
    SbDrmGenerateSessionUpdateRequest(0,1,"cenc","x",1);SbDrmUpdateSession(0,1,"x",1,"s",1);
    SbDrmCloseSession(0,"s",1);SbDrmUpdateServerCertificate(0,1,"x",1);SbDrmDestroySystem(0);
    if(SbMicrophoneGetAvailable(0,0)!=0||SbMicrophoneGetAvailable(0,-1)!=-1||
       SbMicrophoneCreate(0,48000,1024)||SbMicrophoneOpen(0)||SbMicrophoneRead(0,0,0)!=-1)return 22;
    SbMicrophoneDestroy(0);if(SbSpeechSynthesisIsSupported())return 23;
    SbSpeechSynthesisSpeak("unavailable");SbSpeechSynthesisCancel();

    print("ELF system, constants and honest optional-feature contracts PASS\n");
    return 37;
}
