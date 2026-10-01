/* Serialize the sealed musl non-cryptographic PRNG in one guest image. */
#include <stdlib.h>
#include <pthread.h>
int __myos_rand_unlocked(void);
void __myos_srand_unlocked(unsigned);
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
int rand(void)
{
    if(pthread_mutex_lock(&lock))__builtin_trap();
    int value=__myos_rand_unlocked();
    if(pthread_mutex_unlock(&lock))__builtin_trap();
    return value;
}
void srand(unsigned seed)
{
    if(pthread_mutex_lock(&lock))__builtin_trap();
    __myos_srand_unlocked(seed);
    if(pthread_mutex_unlock(&lock))__builtin_trap();
}
