/* Pinned 128-byte mask storage only; this does not supply signal delivery. */
#ifndef MYOS_SDK_SIGNAL_H
#define MYOS_SDK_SIGNAL_H
#include <stdint.h>
#include <errno.h>
typedef struct { uint64_t bits[16]; } sigset_t;
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
#define SIGKILL 9
#define SIGSEGV 11
#define SIGSTOP 19
#define SIGSYS 31
static inline int sigemptyset(sigset_t *set)
{
    if(!set){errno=EINVAL;return -1;}
    for(unsigned i=0;i<16;++i)set->bits[i]=0;
    return 0;
}
static inline int sigfillset(sigset_t *set)
{
    if(sigemptyset(set))return -1;
    set->bits[0]=UINT64_MAX;set->bits[1]=UINT64_MAX>>1;
    return 0;
}
static inline int sigdelset(sigset_t *set,int signal)
{
    if(!set||signal<1||signal>127){errno=EINVAL;return -1;}
    unsigned bit=(unsigned)signal-1;
    set->bits[bit/64]&=~(UINT64_C(1)<<(bit%64));return 0;
}
static inline int sigismember(const sigset_t *set,int signal)
{
    if(!set||signal<1||signal>127){errno=EINVAL;return -1;}
    unsigned bit=(unsigned)signal-1;
    return (int)((set->bits[bit/64]>>(bit%64))&1);
}
#endif
