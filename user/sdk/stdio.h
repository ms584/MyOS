#ifndef MYOS_SDK_STDIO_H
#define MYOS_SDK_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct myos_FILE FILE;
#define EOF (-1)
#ifdef __cplusplus
extern "C" {
#endif
/* Formatter/callback stream subset; file streams and standard streams pending. */
int snprintf(char *,size_t,const char *,...) __attribute__((format(printf,3,4)));
int vsnprintf(char *,size_t,const char *,va_list) __attribute__((format(printf,3,0)));
int asprintf(char **,const char *,...) __attribute__((format(printf,2,3)));
int vasprintf(char **,const char *,va_list) __attribute__((format(printf,2,0)));
int vfprintf(FILE *,const char *,va_list) __attribute__((format(printf,2,0)));
size_t fwrite(const void *,size_t,size_t,FILE *);
#ifdef __cplusplus
}
#endif
#endif
