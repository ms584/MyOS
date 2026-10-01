#ifndef MYOS_SDK_STDIO_H
#define MYOS_SDK_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct myos_FILE FILE;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define _IONBF 2
#ifdef __cplusplus
extern "C" {
#endif
extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;
/* Unbuffered stream profile; standard input and redirection remain unavailable. */
FILE *fopen(const char *,const char *);
FILE *fdopen(int,const char *);
int fclose(FILE *);
size_t fread(void *,size_t,size_t,FILE *);
int fflush(FILE *);
int fseek(FILE *,long,int);
long ftell(FILE *);
int fileno(FILE *);
int feof(FILE *);
int ferror(FILE *);
void clearerr(FILE *);
int setvbuf(FILE *,char *,int,size_t);
int fputc(int,FILE *);
int fgetc(FILE *);
int fputs(const char *,FILE *);
int puts(const char *);
int fprintf(FILE *,const char *,...) __attribute__((format(printf,2,3)));
int printf(const char *,...) __attribute__((format(printf,1,2)));
int vprintf(const char *,va_list) __attribute__((format(printf,1,0)));
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
