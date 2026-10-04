/* Target-only subset of pinned Cobalt 27 LTS pthread opaque ABI.
 * Never include or substitute the host libc's pthread objects here. */
#ifndef MYOS_SDK_PTHREAD_H
#define MYOS_SDK_PTHREAD_H
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
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
typedef union { uint8_t bytes[80];void *alignment; } pthread_rwlock_t;
typedef union { uint8_t bytes[8]; } pthread_rwlockattr_t;
#define PTHREAD_MUTEX_INITIALIZER {{0}}
#define PTHREAD_ONCE_INIT {{0}}
#define PTHREAD_RWLOCK_INITIALIZER {{0}}
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_DESTRUCTOR_ITERATIONS 4
int pthread_create(pthread_t *,const void *,void *(*)(void *),void *);
int pthread_join(pthread_t,void **);
int pthread_detach(pthread_t);
pthread_t pthread_self(void);
int pthread_equal(pthread_t,pthread_t);
int pthread_setname_np(pthread_t,const char *);
int pthread_getname_np(pthread_t,char *,size_t);
int pthread_sigmask(int,const sigset_t *,sigset_t *);
#if defined(__cplusplus) && __cplusplus >= 201103L
[[noreturn]]
#elif defined(__cplusplus)
__attribute__((noreturn))
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
/* Default process-local rwlock attributes only; no pshared/timed/try API. */
int pthread_rwlock_init(pthread_rwlock_t *,const pthread_rwlockattr_t *);
int pthread_rwlock_destroy(pthread_rwlock_t *);
int pthread_rwlock_rdlock(pthread_rwlock_t *);
int pthread_rwlock_wrlock(pthread_rwlock_t *);
int pthread_rwlock_unlock(pthread_rwlock_t *);
#ifdef __cplusplus
}
#if __cplusplus >= 201103L
static_assert(sizeof(pthread_mutex_t)==80&&sizeof(pthread_cond_t)==96&&sizeof(pthread_once_t)==64,"pinned storage");
static_assert(sizeof(pthread_rwlock_t)==80&&alignof(pthread_rwlock_t)==8&&sizeof(pthread_rwlockattr_t)==8,"pinned rwlock storage");
#else
typedef char __myos_pthread_storage[(sizeof(pthread_mutex_t)==80&&sizeof(pthread_cond_t)==96&&sizeof(pthread_once_t)==64)?1:-1];
typedef char __myos_rwlock_storage[(sizeof(pthread_rwlock_t)==80&&__alignof__(pthread_rwlock_t)==8&&sizeof(pthread_rwlockattr_t)==8)?1:-1];
#endif
#else
_Static_assert(sizeof(pthread_mutex_t)==80&&sizeof(pthread_cond_t)==96&&sizeof(pthread_once_t)==64,"pinned storage");
_Static_assert(_Alignof(pthread_mutex_t)==8&&_Alignof(pthread_cond_t)==8,"pinned alignment");
_Static_assert(sizeof(pthread_rwlock_t)==80&&_Alignof(pthread_rwlock_t)==8&&sizeof(pthread_rwlockattr_t)==8,"pinned rwlock storage");
#endif
#endif
