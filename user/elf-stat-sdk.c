#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "os_file.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"stat SDK failed: %s\n",#x);__builtin_trap();}}while(0)
static void *held[20000];
/* Test-only linker wrappers delegate to actual native runtime except injection. */
void *__real_malloc(size_t);
void *__real_realloc(void *,size_t);
static int fail_malloc,fail_realloc;
void *__wrap_malloc(size_t size)
{if(fail_malloc){errno=ENOMEM;return 0;}return __real_malloc(size);}
void *__wrap_realloc(void *pointer,size_t size)
{if(fail_realloc){errno=ENOMEM;return 0;}return __real_realloc(pointer,size);}
int _start(void)
{
    const char *path="/sdk-stat";unlink(path);
    FILE *f=os_file_create_unique(path,0600);CHECK(f);
    CHECK(!os_file_create_unique(path,0600)&&errno==EEXIST);
    unsigned char payload[4096];for(unsigned i=0;i<sizeof(payload);++i)payload[i]=(unsigned char)i;
    CHECK(fwrite(payload,1,sizeof(payload),f)==sizeof(payload)&&!fclose(f));
    struct {uint64_t before;struct stat info;uint64_t after;} guarded;
    memset(&guarded,0xa5,sizeof(guarded));
    CHECK(!stat(path,&guarded.info));
    CHECK(guarded.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&guarded.after==guarded.before);
    CHECK(S_ISREG(guarded.info.st_mode)&&guarded.info.st_size==4096);
    CHECK(guarded.info.st_blksize==4096&&guarded.info.st_blocks==8);
    int fd=open(path,O_RDONLY);CHECK(fd>=0);
    struct stat info;CHECK(!fstat(fd,&info)&&!memcmp(&info,&guarded.info,sizeof(info)));
    CHECK(!fstatat(AT_FDCWD,path,&info,0));
    struct stat preserved=info;
    CHECK(fstat(-1,&info)==-1&&errno==EBADF&&!memcmp(&info,&preserved,sizeof(info)));
    CHECK(fstatat(AT_FDCWD,path,&info,1)==-1&&errno==EINVAL);
    CHECK(!memcmp(&info,&preserved,sizeof(info)));
    CHECK(stat("/sdk-missing",&info)==-1&&errno==ENOENT);
    CHECK(!memcmp(&info,&preserved,sizeof(info)));
    int copy=os_dupfd_cloexec(fd);CHECK(copy>=0&&fcntl(copy,F_GETFD)==FD_CLOEXEC);
    CHECK(read(fd,payload,7)==7&&lseek(copy,0,SEEK_CUR)==7);
    CHECK(!close(fd)&&!fstat(copy,&info)&&info.st_size==4096&&!close(copy));
    size_t size=99;char *data=os_read_file(path,&size);
    CHECK(data&&size==4096&&!data[4096]);
    for(unsigned i=0;i<4096;++i)CHECK((unsigned char)data[i]==(unsigned char)i);
    free(data);
    fail_malloc=1;f=os_file_create_unique("/sdk-stat-fail",0600);fail_malloc=0;
    CHECK(!f&&errno==ENOMEM&&!unlink("/sdk-stat-fail"));
    fail_malloc=1;size=99;data=os_read_file(path,&size);fail_malloc=0;
    CHECK(!data&&errno==ENOMEM&&size==99);
    fail_realloc=1;size=99;data=os_read_file(path,&size);fail_realloc=0;
    CHECK(!data&&errno==ENOMEM&&size==99);
    CHECK(!mkdir("/sdk-stat-dir",0700));
    CHECK(!stat("/sdk-stat-dir",&info)&&S_ISDIR(info.st_mode));
    fd=open("/sdk-stat-dir",O_RDONLY|O_DIRECTORY);CHECK(fd>=0);
    CHECK(!fstat(fd,&info)&&S_ISDIR(info.st_mode)&&!close(fd));
    CHECK(!unlinkat(AT_FDCWD,"/sdk-stat-dir",AT_REMOVEDIR));
    CHECK(!stat("/assets/stat",&info)&&S_ISREG(info.st_mode)&&info.st_size>0);
    CHECK(!(info.st_mode&S_IWUSR));
    size_t n=0;while(n<20000&&(held[n]=malloc(131072)))++n;
    while(n<20000&&(held[n]=malloc(1)))++n;
    CHECK(n<20000&&n);
    free(held[--n]);size=99;
    CHECK(!os_read_file(path,&size)&&errno==ENOMEM&&size==99);
    while(n)free(held[--n]);
    int files[8];for(unsigned i=0;i<8;++i){files[i]=open(path,O_RDONLY);CHECK(files[i]>=0);}
    for(unsigned i=0;i<8;++i)CHECK(!close(files[i]));
    CHECK(!unlink(path)&&stat(path,&info)==-1&&errno==ENOENT);
    puts("native stat SDK/real Mesa file/metadata/dup/OOM PASS");return 83;
}
