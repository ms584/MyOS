#include <netdb.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "dns-call.h"
extern void SbLogRaw(const char *);
extern int pthread_kill(pthread_t,int);
#ifndef MODE
#define MODE 0
#endif
#define CHECK(x) do{if(!(x)){SbLogRaw("DNS failed: " #x "\n");__builtin_trap();}}while(0)
struct address {uint16_t family;uint8_t port[2],ip[4],pad[8];};
static bool equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int64_t call(unsigned op,uint64_t a,uint64_t b)
{register uint64_t r __asm__("rax")=MYOS_DNS_CALL;
 __asm__ volatile("int $0x80":"+a"(r):"D"((uint64_t)op),"S"(a),"d"(b):"memory","cc");return (int64_t)r;}
static void pause_ms(unsigned n){struct timespec t={n/1000,(long)(n%1000)*1000000};CHECK(!clock_nanosleep(1,0,&t,0));}
static void verify(const char *host,unsigned count,const char *canonical)
{
 struct addrinfo hints={.ai_family=2,.ai_socktype=1,.ai_flags=AI_CANONNAME};
 struct addrinfo *result=0;CHECK(!getaddrinfo(host,"443",&hints,&result));CHECK(result);
 if(canonical)CHECK(result->ai_canonname&&equal(result->ai_canonname,canonical));
 unsigned seen=0;
 for(struct addrinfo *p=result;p;p=p->ai_next){CHECK(p->ai_family==2&&p->ai_socktype==1&&p->ai_protocol==6&&p->ai_addrlen==16);
  struct address *a=(void *)p->ai_addr;CHECK(a->family==2&&a->port[0]==1&&a->port[1]==187);
  CHECK(a->ip[0]==192&&a->ip[1]==0&&a->ip[2]==2&&a->ip[3]==10+seen);++seen;}
 CHECK(seen==count);freeaddrinfo(result);
}
static void numeric_tests(void)
{
 const char *names[]={"127.0.0.1","127.1","0x7f000001","0177.0.0.1","2130706433","localhost"};
 struct addrinfo hints={.ai_family=2,.ai_socktype=1};
 for(unsigned i=0;i<6;++i){struct addrinfo *p=0;CHECK(!getaddrinfo(names[i],"https",&hints,&p));
  struct address *a=(void *)p->ai_addr;CHECK(a->ip[0]==127&&!a->ip[1]&&!a->ip[2]&&a->ip[3]==1);
  CHECK(!p->ai_next&&a->port[0]==1&&a->port[1]==187);freeaddrinfo(p);}
 struct addrinfo *out=(void *)1;
 CHECK(getaddrinfo(0,0,0,&out)==EAI_NONAME&&out==(void *)1);
 hints.ai_family=10;CHECK(getaddrinfo("127.0.0.1","80",&hints,&out)==EAI_FAMILY&&out==(void *)1);
 hints.ai_family=2;hints.ai_flags=0x800;CHECK(getaddrinfo("127.0.0.1","80",&hints,&out)==EAI_BADFLAGS);
 hints.ai_flags=AI_NUMERICHOST;CHECK(getaddrinfo("multi.test","80",&hints,&out)==EAI_NONAME);
 hints.ai_flags=AI_NUMERICSERV;CHECK(getaddrinfo("127.0.0.1","https",&hints,&out)==EAI_NONAME);
 hints.ai_flags=0;CHECK(getaddrinfo("127.0.0.1","65536",&hints,&out)==EAI_SERVICE);
 CHECK(getaddrinfo("127.0.0.1","",&hints,&out)==EAI_SERVICE);
 CHECK(getaddrinfo("127.0.0.1","80",&hints,0)==EAI_SYSTEM&&errno==EFAULT);
 CHECK(gai_strerror(EAI_AGAIN)&&gai_strerror(-999));freeaddrinfo(0);
}
static volatile uint64_t progress;static volatile unsigned stop;
static void *counter(void *unused){(void)unused;while(!__atomic_load_n(&stop,__ATOMIC_ACQUIRE)){pause_ms(10);++progress;}return 0;}
static void *query_worker(void *unused){(void)unused;
 if(MODE==2){struct addrinfo *result=0;(void)getaddrinfo("block.test","80",0,&result);__builtin_trap();}
 verify("multi.test",2,"multi.test");return (void *)91;}
static uint32_t foreign_handle;
static void *foreign(void *unused){(void)unused;struct dns_job_status status={0};
 CHECK(call(DNS_CALL_STATUS,foreign_handle,(uintptr_t)&status)==-9);return (void *)92;}
static void *interrupted(void *unused){(void)unused;struct addrinfo *result=0;
 (void)result;volatile uint64_t sequence=0;
 struct dns_start_request request={.sequence=(uintptr_t)&sequence,.name="block.test"};
 CHECK(call(DNS_CALL_START,(uintptr_t)&request,0)>0);
 /* Exit with a live kernel-owned query; stop-thread must release it even when
  * no user close/cleanup runs. Nonzero cross-thread pthread_kill is still open. */
 pthread_exit((void *)93);}
static void private_tests(void)
{
 volatile uint64_t sequence=0;struct dns_start_request request={.sequence=(uintptr_t)&sequence,.name="multi.test"};
 CHECK(call(99,0,0)==-22&&call(DNS_CALL_CONFIGURED,1,0)==-22);
 CHECK(call(DNS_CALL_START,UINT64_MAX-8,0)==-14);
 request.reserved=1;CHECK(call(DNS_CALL_START,(uintptr_t)&request,0)==-22);request.reserved=0;
 request.sequence=(uintptr_t)"readonly";CHECK(call(DNS_CALL_START,(uintptr_t)&request,0)==-14);
 request.sequence=(uintptr_t)&sequence;
 int64_t h=call(DNS_CALL_START,(uintptr_t)&request,0);CHECK(h>0);
 foreign_handle=(uint32_t)h;pthread_t thread;CHECK(!pthread_create(&thread,0,foreign,0));void *value;
 CHECK(!pthread_join(thread,&value)&&value==(void *)92);
 CHECK(call(DNS_CALL_STATUS,(uint64_t)1<<32,(uintptr_t)&sequence)==-9);
 CHECK(call(DNS_CALL_STATUS,(uint32_t)h,(uintptr_t)"readonly")==-14);
 CHECK(!call(DNS_CALL_CLOSE,(uint32_t)h,0)&&call(DNS_CALL_CLOSE,(uint32_t)h,0)==-9);
 int64_t newer=call(DNS_CALL_START,(uintptr_t)&request,0);CHECK(newer>0&&newer!=h);
 struct dns_job_status status;CHECK(call(DNS_CALL_STATUS,(uint32_t)h,(uintptr_t)&status)==-9);
 CHECK(!call(DNS_CALL_CLOSE,(uint32_t)newer,0));
 uint32_t handles[16];for(unsigned i=0;i<16;++i){h=call(DNS_CALL_START,(uintptr_t)&request,0);CHECK(h>0);handles[i]=(uint32_t)h;}
 CHECK(call(DNS_CALL_START,(uintptr_t)&request,0)==-11);
 for(unsigned i=0;i<16;++i)CHECK(!call(DNS_CALL_CLOSE,handles[i],0));
}
int _start(void)
{
 if(MODE==0){numeric_tests();struct addrinfo *out=(void *)1;
  CHECK(getaddrinfo("multi.test","80",0,&out)==EAI_AGAIN&&out==(void *)1);
  SbLogRaw("DNS offline numeric/errors PASS\n");return 89;}
 if(MODE==3){struct addrinfo *out=(void *)1;
  CHECK(getaddrinfo("multi.test","80",0,&out)==EAI_SYSTEM&&errno==ENOSYS&&out==(void *)1);
  SbLogRaw("DNS entropy fail-closed PASS\n");return 89;}
 pthread_t threads[8];
 if(MODE==2){for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,query_worker,0));
  pause_ms(200);SbLogRaw("DNS blocked workers ready\n");for(;;)pause_ms(1000);}
 numeric_tests();private_tests();verify("multi.test",2,"multi.test");verify("cname.test",1,"target.test");
 verify("wrong.test",1,"wrong.test");verify("tcp.test",1,"tcp.test");
 for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,query_worker,0));
 for(unsigned i=0;i<8;++i){void *value;CHECK(!pthread_join(threads[i],&value)&&value==(void *)91);}
 struct addrinfo *out=(void *)1;CHECK(getaddrinfo("missing.test","80",0,&out)==EAI_NONAME&&out==(void *)1);
 pthread_t tick;CHECK(!pthread_create(&tick,0,counter,0));
 CHECK(getaddrinfo("timeout.test","80",0,&out)==EAI_AGAIN&&out==(void *)1);
 __atomic_store_n(&stop,1,__ATOMIC_RELEASE);CHECK(!pthread_join(tick,0)&&progress>=10);
 for(unsigned i=0;i<18;++i){pthread_t worker;CHECK(!pthread_create(&worker,0,interrupted,0));
  void *value;CHECK(!pthread_join(worker,&value)&&value==(void *)93);}
 verify("multi.test",2,"multi.test");
 SbLogRaw("DNS native syscall/concurrent/CNAME/wrong-ID/TCP/timeout/worker-cleanup PASS\n");return 89;
}
