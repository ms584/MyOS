/* Private adapter for sealed musl formatting, not the pinned core's FILE ABI. */
#ifndef MYOS_FORMAT_INTERNAL_H
#define MYOS_FORMAT_INTERNAL_H
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#define hidden __attribute__((visibility("hidden")))
#define weak_alias(old,new) extern __typeof(old) new __attribute__((weak,alias(#old)))
#define F_NOWR 8
#define F_ERR 32
struct myos_FILE {
    unsigned flags;
    unsigned char *rpos,*rend,*wpos,*wbase,*wend,*buf;
    size_t buf_size;
    size_t (*write)(FILE *,const unsigned char *,size_t);
    int lock,lbf,mode;
    void *cookie;
    pthread_mutex_t mutex;
};
int __lockfile(FILE *);
void __unlockfile(FILE *);
int __towrite(FILE *);
size_t __fwritex(const unsigned char *,size_t,FILE *);
#define FLOCK(f) int __need_unlock=((f)->lock>=0?__lockfile(f):0)
#define FUNLOCK(f) do{if(__need_unlock)__unlockfile(f);}while(0)
#define ferror(f) ((f)->flags&F_ERR)
#endif
