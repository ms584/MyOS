#ifndef MYOS_SDK_STDLIB_H
#define MYOS_SDK_STDLIB_H
#include <stddef.h>
#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#include <alloca.h>
#endif
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
int mkstemp(char *);
char *mkdtemp(char *);
#define RAND_MAX 2147483647
int rand(void);
void srand(unsigned);
void qsort(void *,size_t,size_t,int (*)(const void *,const void *));
void qsort_r(void *,size_t,size_t,int (*)(const void *,const void *,void *),void *);
int atexit(void (*)(void));
long strtol(const char *,char **,int);
unsigned long strtoul(const char *,char **,int);
long long strtoll(const char *,char **,int);
unsigned long long strtoull(const char *,char **,int);
float strtof(const char *,char **);
double strtod(const char *,char **);
long double strtold(const char *,char **);
int atoi(const char *);
long atol(const char *);
long long atoll(const char *);
double atof(const char *);
int abs(int);
long labs(long);
long long llabs(long long);
void *bsearch(const void *,const void *,size_t,size_t,int (*)(const void *,const void *));
#ifdef __cplusplus
}
#endif
#endif
