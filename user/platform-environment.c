/* Process-local target SDK. Never read the WSL/host environment.
 * Immutable copied strings retain returned-pointer validity until ELF teardown.
 * Storage is bounded, including retired values: 64 active keys, 64 KiB total.
 */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#define ENV_KEYS 64
#define ENV_BYTES 65536
static struct {const char *key;char *value;} entries[ENV_KEYS];
static char storage[ENV_BYTES];
static size_t used;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static void acquire(void){if(pthread_mutex_lock(&lock))__builtin_trap();}
static void release(void){if(pthread_mutex_unlock(&lock))__builtin_trap();}
static int equal(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int valid(const char *key)
{if(!key||!*key)return 0;for(;*key;++key)if(*key=='=')return 0;return 1;}
static char *copy(const char *text,size_t length)
{char *target=storage+used;memcpy(target,text,length);target[length]=0;used+=length+1;return target;}
char *getenv(const char *key)
{
    if(!valid(key))return 0;
    char *value=0;acquire();
    for(unsigned i=0;i<ENV_KEYS;++i)if(entries[i].key&&equal(entries[i].key,key)){
        value=entries[i].value;break;
    }
    release();return value;
}
int setenv(const char *key,const char *value,int overwrite)
{
    if(!valid(key)||!value){errno=EINVAL;return -1;}
    acquire();unsigned index=ENV_KEYS,empty=ENV_KEYS;
    for(unsigned i=0;i<ENV_KEYS;++i){
        if(!entries[i].key){if(empty==ENV_KEYS)empty=i;}
        else if(equal(entries[i].key,key)){index=i;break;}
    }
    if(index!=ENV_KEYS&&!overwrite){release();return 0;}
    size_t value_length=strlen(value),key_length=index==ENV_KEYS?strlen(key):0;
    size_t available=ENV_BYTES-used;
    if((index==ENV_KEYS&&empty==ENV_KEYS)||value_length>=available||
       (index==ENV_KEYS&&(available-value_length-1<=key_length))){
        release();errno=ENOMEM;return -1;
    }
    char *new_value=copy(value,value_length);
    if(index==ENV_KEYS){index=empty;entries[index].key=copy(key,key_length);}
    entries[index].value=new_value;release();return 0;
}
int unsetenv(const char *key)
{
    if(!valid(key)){errno=EINVAL;return -1;}
    acquire();for(unsigned i=0;i<ENV_KEYS;++i)if(entries[i].key&&equal(entries[i].key,key)){
        entries[i].key=0;entries[i].value=0;break;
    }
    release();return 0;
}
/* Same single guest identity as the runtime's getuid/geteuid. No credentials,
 * group mutation, set-id execution or permission enforcement is advertised. */
gid_t getgid(void){return 0;}
gid_t getegid(void){return 0;}
