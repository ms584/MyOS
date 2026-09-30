/* Test-only pinned Cobalt 27 LTS SABI objects, not the host pthread ABI.
 * Source: starboard_layer_posix_pthread_abi_wrappers.h at
 * youtube/cobalt 3befd332486952f6603ae3baf2a8713878d6e200.
 */
#ifndef MYOS_EMUTLS_TEST_PTHREAD_H
#define MYOS_EMUTLS_TEST_PTHREAD_H
#include <stddef.h>
#include <stdint.h>
typedef void *pthread_t;
typedef void *pthread_key_t;
typedef union { uint8_t bytes[80]; void *pointer; } pthread_mutex_t;
typedef union { uint8_t bytes[64]; void *pointer; } pthread_once_t;
#define PTHREAD_MUTEX_INITIALIZER {{0}}
#define PTHREAD_ONCE_INIT {{0}}
_Static_assert(sizeof(pthread_key_t)==8,"Cobalt key pointer width");
_Static_assert(sizeof(pthread_mutex_t)==80,"Cobalt mutex storage");
_Static_assert(sizeof(pthread_once_t)==64,"Cobalt once storage");
int pthread_key_create(pthread_key_t *,void (*)(void *));
int pthread_key_delete(pthread_key_t);
void *pthread_getspecific(pthread_key_t);
int pthread_setspecific(pthread_key_t,const void *);
int pthread_once(pthread_once_t *,void (*)(void));
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_create(pthread_t *,const void *,void *(*)(void *),void *);
int pthread_join(pthread_t,void **);
int pthread_detach(pthread_t);
int pthread_kill(pthread_t,int);
#endif
