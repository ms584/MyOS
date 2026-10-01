/* Process-local SDK registration; no host libc, TLS destructors or unwinding. */
#include <pthread.h>
#include <stdlib.h>
struct registration { struct registration *next;void (*function)(void *);void *argument,*dso; };
static struct registration *head;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static void acquire(void){if(pthread_mutex_lock(&lock))__builtin_trap();}
static void release(void){if(pthread_mutex_unlock(&lock))__builtin_trap();}
int __cxa_atexit(void (*function)(void *),void *argument,void *dso)
{
    if(!function)return -1;
    struct registration *entry=malloc(sizeof(*entry));
    if(!entry)return -1;
    *entry=(struct registration){0,function,argument,dso};
    acquire();entry->next=head;head=entry;release();return 0;
}
void __cxa_finalize(void *dso)
{
    for(;;){
        acquire();struct registration **position=&head;
        while(*position&&dso&&(*position)->dso!=dso)position=&(*position)->next;
        struct registration *entry=*position;
        if(entry)*position=entry->next;
        release();if(!entry)return;
        /* Unlink before callback: recursive/concurrent finalize cannot run it twice.
         * Callbacks may register another destructor without holding this lock. */
        entry->function(entry->argument);free(entry);
    }
}
struct simple { void (*function)(void); };
static void invoke_simple(void *raw)
{
    struct simple *entry=raw;entry->function();free(entry);
}
int atexit(void (*function)(void))
{
    if(!function)return -1;
    struct simple *entry=malloc(sizeof(*entry));if(!entry)return -1;
    entry->function=function;
    if(__cxa_atexit(invoke_simple,entry,0)){free(entry);return -1;}
    return 0;
}
/* One owned ELF image. Compiler registration passes &__dso_handle. */
__attribute__((visibility("hidden"))) void *__dso_handle=&__dso_handle;
__attribute__((destructor(101))) static void shutdown(void){__cxa_finalize(0);}
