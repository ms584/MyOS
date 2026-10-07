#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include "../include/thread-limit.h"
extern int sem_init(void *,int,unsigned),sem_wait(void *),sem_post(void *),sem_destroy(void *),*__errno_location(void);
extern long sysconf(int);
extern const uint32_t kSbMaxThreads;
extern int64_t write(int,const void *,size_t);
static uint64_t ready[4],gate[4];
static _Thread_local uint64_t initialized=0x13572468;
static uintptr_t pointers[MYOS_WORKER_LIMIT];
static pthread_key_t key;
static void *worker(void *raw)
{
 unsigned index=(unsigned)(uintptr_t)raw;
 if(initialized!=0x13572468||pthread_setspecific(key,(void *)(uintptr_t)(index+1)))return (void *)1;
 pointers[index]=(uintptr_t)&initialized;initialized=index+99;
 if(sem_post(ready)||sem_wait(gate)||initialized!=index+99||pthread_getspecific(key))return (void *)2;
 return (void *)(uintptr_t)(index+10);
}
int _start(void)
{
 if(kSbMaxThreads!=MYOS_THREAD_LIMIT||sysconf(76)!=MYOS_THREAD_LIMIT)return 1;
 pthread_attr_t attr;if(pthread_attr_init(&attr)||pthread_attr_setstacksize(&attr,65536))return 2;
 if(sem_init(ready,0,0)||sem_init(gate,0,0))return 3;
 pthread_t threads[MYOS_WORKER_LIMIT],extra;
 for(unsigned round=0;round<3;++round){
  if(pthread_key_create(&key,0))return 12;
  for(uintptr_t i=0;i<MYOS_WORKER_LIMIT;++i)if(pthread_create(&threads[i],&attr,worker,(void *)i))return 4;
  for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)if(sem_wait(ready))return 5;
  for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i){if(!pointers[i]||pointers[i]==(uintptr_t)&initialized)return 6;
   for(unsigned j=0;j<i;++j)if(pointers[i]==pointers[j])return 7;}
  *__errno_location()=77;extra=(pthread_t)123;
  if(pthread_create(&extra,&attr,worker,0)!=11||extra!=(pthread_t)123||*__errno_location()!=77)return 8;
  if(pthread_key_delete(key)||pthread_key_create(&key,0))return 13;
  for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i)if(sem_post(gate))return 9;
  for(unsigned i=0;i<MYOS_WORKER_LIMIT;++i){void *result;
   if(pthread_join(threads[i],&result)||result!=(void *)(uintptr_t)(i+10))return 10;}
  if(pthread_key_delete(key))return 14;
 }
 if(initialized!=0x13572468||sem_destroy(ready)||sem_destroy(gate)||pthread_attr_destroy(&attr))return 11;
 const char message[]="native16 workers saturation/TLS/reuse/errno PASS\n";write(1,message,sizeof(message)-1);return 92;
}
