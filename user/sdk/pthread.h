/* Target-only subset of pinned Cobalt 27 LTS pthread opaque ABI.
 * Never include or substitute the host libc's pthread objects here. */
#ifndef MYOS_SDK_PTHREAD_H
#define MYOS_SDK_PTHREAD_H
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void *pthread_t;
typedef void *pthread_key_t;
typedef union { uint8_t bytes[80];void *alignment; } pthread_mutex_t;
typedef union { uint8_t bytes[40];void *alignment; } pthread_mutexattr_t;
typedef union { uint8_t bytes[96];void *alignment; } pthread_cond_t;
typedef union { uint8_t bytes[40];void *alignment; } pthread_condattr_t;
typedef union { uint8_t bytes[64];void *alignment; } pthread_once_t;
#define PTHREAD_MUTEX_INITIALIZER {{0}}
#define PTHREAD_ONCE_INIT {{0}}
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_DESTRUCTOR_ITERATIONS 4
int pthread_create(pthread_t *,const void *,void *(*)(void *),void *);
int pthread_join(pthread_t,void **);
int pthread_detach(pthread_t);
pthread_t pthread_self(void);
int pthread_equal(pthread_t,pthread_t);
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void pthread_exit(void *);
int pthread_once(pthread_once_t *,void (*)(void));
int pthread_mutex_init(pthread_mutex_t *,const pthread_mutexattr_t *);
int pthread_mutex_destroy(pthread_mutex_t *);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_trylock(pthread_mutex_t *);
int pthread_mutex_timedlock(pthread_mutex_t *,const struct timespec *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_mutexattr_init(pthread_mutexattr_t *);
int pthread_mutexattr_destroy(pthread_mutexattr_t *);
int pthread_mutexattr_settype(pthread_mutexattr_t *,int);
int pthread_cond_init(pthread_cond_t *,const pthread_condattr_t *);
int pthread_cond_destroy(pthread_cond_t *);
int pthread_cond_wait(pthread_cond_t *,pthread_mutex_t *);
int pthread_cond_timedwait(pthread_cond_t *,pthread_mutex_t *,const struct timespec *);
int pthread_cond_signal(pthread_cond_t *);
int pthread_cond_broadcast(pthread_cond_t *);
int pthread_condattr_init(pthread_condattr_t *);
int pthread_condattr_destroy(pthread_condattr_t *);
int pthread_condattr_setclock(pthread_condattr_t *,int);
int pthread_key_create(pthread_key_t *,void (*)(void *));
int pthread_key_delete(pthread_key_t);
void *pthread_getspecific(pthread_key_t);
int pthread_setspecific(pthread_key_t,const void *);
#ifdef __cplusplus
}
static_assert(sizeof(pthread_mutex_t)==80&&sizeof(pthread_cond_t)==96&&sizeof(pthread_once_t)==64,"pinned storage");
#else
_Static_assert(sizeof(pthread_mutex_t)==80&&sizeof(pthread_cond_t)==96&&sizeof(pthread_once_t)==64,"pinned storage");
_Static_assert(_Alignof(pthread_mutex_t)==8&&_Alignof(pthread_cond_t)==8,"pinned alignment");
#endif
#endif
