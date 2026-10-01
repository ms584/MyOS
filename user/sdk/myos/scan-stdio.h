/* PRIVATE memory-scanner state, never a public FILE or Cobalt ABI. */
#ifndef MYOS_MEMORY_SCAN_STDIO_H
#define MYOS_MEMORY_SCAN_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#define hidden __attribute__((visibility("hidden")))
#define weak_alias(old,new) extern __typeof(old) new __attribute__((weak,alias(#old)))
#define EOF (-1)
typedef struct myos_memory_scan {
    unsigned char *buf,*rpos,*rend,*shend;
    off_t shlim,shcnt;
    void *cookie;
    size_t (*read)(struct myos_memory_scan *,unsigned char *,size_t);
    int lock;
} FILE;
#define FLOCK(f) ((void)(f))
#define FUNLOCK(f) ((void)(f))
hidden int __myos_memory_toread(FILE *);
hidden int __myos_memory_uflow(FILE *);
#define __toread __myos_memory_toread
#define __uflow __myos_memory_uflow
hidden int __myos_memory_vfscanf(FILE *,const char *,va_list);
#define vfscanf __myos_memory_vfscanf
int sscanf(const char *,const char *,...);
int vsscanf(const char *,const char *,va_list);
#endif
