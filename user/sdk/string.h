#ifndef MYOS_SDK_STRING_H
#define MYOS_SDK_STRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *memcpy(void *,const void *,size_t);
void *memset(void *,int,size_t);
void *memchr(const void *,int,size_t);
void *memmove(void *,const void *,size_t);
int memcmp(const void *,const void *,size_t);
size_t strlen(const char *);
size_t strnlen(const char *,size_t);
int strcmp(const char *,const char *);
char *strchr(const char *,int);
char *strncpy(char *,const char *,size_t);
char *strstr(const char *,const char *);
char *strdup(const char *);
char *strerror(int);
#ifdef __cplusplus
}
#endif
#endif
