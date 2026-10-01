#ifndef MYOS_SDK_STDLIB_H
#define MYOS_SDK_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *malloc(size_t);
void *calloc(size_t,size_t);
void *aligned_alloc(size_t,size_t);
int posix_memalign(void **,size_t,size_t);
void *realloc(void *,size_t);
void free(void *);
char *getenv(const char *);
int setenv(const char *,const char *,int);
int unsetenv(const char *);
#ifdef __cplusplus
}
#endif
#endif
