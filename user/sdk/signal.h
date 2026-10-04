/* Pinned x64 Cobalt/musl layouts and mask helpers only.
 * No signal/sigaction/raise/pause provider or signal delivery is supplied. */
#ifndef MYOS_SDK_SIGNAL_H
#define MYOS_SDK_SIGNAL_H
#include <stdint.h>
#include <errno.h>
typedef struct { uint64_t bits[16]; } sigset_t;
union sigval { int sival_int;void *sival_ptr; };
typedef struct {
    int si_signo,si_errno,si_code;
    union {
        char padding[112];
        struct {
            union { struct { int pid;unsigned uid; } identity;
                    struct { int timerid,overrun; } timer; } first;
            union { union sigval value;
                    struct { int status;long utime,stime; } child; } second;
        } common;
        struct {
            void *address;short address_lsb;
            union { struct { void *lower,*upper; } bounds;unsigned pkey; } first;
        } fault;
        struct { long band;int fd; } poll;
        struct { void *call_address;int syscall;unsigned arch; } system;
    } fields;
} siginfo_t;
struct sigaction {
    union { void (*sa_handler)(int);void (*sa_sigaction)(int,siginfo_t *,void *); };
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};
/* Constants describe the pinned ABI, not currently supported delivery modes. */
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)
#define SIGILL 4
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGUSR1 10
#define SIGUSR2 12
#define SIGTERM 15
#define SA_NOCLDSTOP 1
#define SA_NOCLDWAIT 2
#define SA_SIGINFO 4
#define SA_ONSTACK 0x08000000
#define SA_RESTART 0x10000000
#define SA_NODEFER 0x40000000
#define SA_RESETHAND 0x80000000u
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
static inline int sigaddset(sigset_t *set,int signal)
{
    if(!set||signal<1||signal>127){errno=EINVAL;return -1;}
    unsigned bit=(unsigned)signal-1;
    set->bits[bit/64]|=UINT64_C(1)<<(bit%64);return 0;
}
static inline int sigismember(const sigset_t *set,int signal)
{
    if(!set||signal<1||signal>127){errno=EINVAL;return -1;}
    unsigned bit=(unsigned)signal-1;
    return (int)((set->bits[bit/64]>>(bit%64))&1);
}
#endif
