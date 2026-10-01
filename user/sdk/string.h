#ifndef MYOS_SDK_STRING_H
#define MYOS_SDK_STRING_H
#include <stddef.h>
#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#include <strings.h>
#endif
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
int strncmp(const char *,const char *,size_t);
char *strcpy(char *,const char *);
char *strcat(char *,const char *);
char *strncat(char *,const char *,size_t);
char *strchr(const char *,int);
char *strchrnul(const char *,int);
char *strrchr(const char *,int);
char *strpbrk(const char *,const char *);
size_t strspn(const char *,const char *);
size_t strcspn(const char *,const char *);
char *strtok(char *,const char *);
char *strtok_r(char *,const char *,char **);
char *strncpy(char *,const char *,size_t);
char *strstr(const char *,const char *);
char *strdup(const char *);
char *strndup(const char *,size_t);
char *strerror(int);
#ifdef __cplusplus
}
#endif
#endif
