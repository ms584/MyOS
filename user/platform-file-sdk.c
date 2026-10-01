/* Source adapters to existing runtime operations; no host syscalls/metadata. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
int open(const char *path,int flags,...)
{
    mode_t mode=0;
    if(flags&O_CREAT){va_list args;va_start(args,flags);mode=va_arg(args,mode_t);va_end(args);}
    return openat(AT_FDCWD,path,flags,mode);
}
int stat(const char *path,struct stat *information)
{return fstatat(AT_FDCWD,path,information,0);}
int unlink(const char *path){return unlinkat(AT_FDCWD,path,0);}
