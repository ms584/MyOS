#ifndef MYOS_SDK_UNISTD_H
#define MYOS_SDK_UNISTD_H
#include <sys/types.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#ifdef __cplusplus
extern "C" {
#endif
/* Supported subsets and bounds are recorded in PHASE331_FILES.md. */
ssize_t read(int,void *,size_t);
ssize_t write(int,const void *,size_t);
ssize_t pread(int,void *,size_t,off_t);
ssize_t pwrite(int,const void *,size_t,off_t);
off_t lseek(int,off_t,int);
int close(int);
int dup(int);
int pipe(int [2]);
int pipe2(int [2],int);
int ftruncate(int,off_t);
int fsync(int);
int fdatasync(int);
int unlinkat(int,const char *,int);
int access(const char *,int);
int isatty(int);
char *getcwd(char *,size_t);
long pathconf(const char *,int);
pid_t getpid(void);
uid_t getuid(void);
uid_t geteuid(void);
#ifdef __cplusplus
}
#endif
#endif
