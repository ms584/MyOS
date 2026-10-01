#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include "anon_file.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"unlink failed: %s\n",#x);__builtin_trap();}}while(0)
#ifdef UNLINK_CANCEL
static int ends[2];
static void *reader(void *arg)
{(void)arg;char c;puts("unlink blocked reader ready");read(ends[0],&c,1);__builtin_trap();}
#endif
int _start(void)
{
#ifdef UNLINK_SNAPSHOT
    int saved=open("/unlink-saved",O_RDONLY);
    char buffer[8];struct stat info;
    if(saved>=0){
        CHECK(read(saved,buffer,5)==5&&!memcmp(buffer,"saved",5)&&!close(saved));
        CHECK(stat("/unlink-ghost",&info)==-1&&errno==ENOENT);
        puts("native orphan snapshot reboot PASS");return 85;
    }
    CHECK(errno==ENOENT);
    int orphan=open("/unlink-ghost",O_CREAT|O_EXCL|O_RDWR,0600);CHECK(orphan>=0);
    CHECK(!ftruncate(orphan,8192)&&!unlink("/unlink-ghost"));
    saved=open("/unlink-saved",O_CREAT|O_EXCL|O_RDWR,0600);CHECK(saved>=0);
    CHECK(write(saved,"saved",5)==5&&!fsync(saved));
    CHECK(!fstat(orphan,&info)&&info.st_size==8192);
    CHECK(!close(saved)&&!close(orphan));
    puts("native orphan snapshot commit PASS");return 85;
#endif
    const char *path="/unlink-sdk";
    unlink(path);
    int fd=open(path,O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fd>=0);
    char data[4096];memset(data,'A',sizeof(data));
    CHECK(write(fd,data,sizeof(data))==sizeof(data));
    int copy=dup(fd);CHECK(copy>=0);
    int independent=open(path,O_RDONLY);CHECK(independent>=0);
    CHECK(!unlink(path));struct stat st;
    CHECK(stat(path,&st)==-1&&errno==ENOENT);
    CHECK(open(path,O_RDONLY)==-1&&errno==ENOENT);
    CHECK(!fstat(fd,&st)&&st.st_size==4096);
#ifdef UNLINK_CANCEL
    CHECK(!pipe(ends));pthread_t worker;
    CHECK(!pthread_create(&worker,0,reader,0));
    void *result;pthread_join(worker,&result);__builtin_trap();
#endif
    int fresh=open(path,O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fresh>=0);
    CHECK(write(fresh,"new",3)==3);
    CHECK(read(independent,data,3)==3&&!memcmp(data,"AAA",3));
    CHECK(!close(fd)&&lseek(copy,0,SEEK_SET)==0);
    CHECK(write(copy,"old",3)==3&&lseek(independent,0,SEEK_SET)==0);
    CHECK(read(independent,data,3)==3&&!memcmp(data,"old",3));
    CHECK(lseek(fresh,0,SEEK_SET)==0&&read(fresh,data,3)==3&&!memcmp(data,"new",3));
    CHECK(!close(copy)&&!close(independent)&&!close(fresh)&&!unlink(path));
    /* More than the RAMFS node count: final close must recycle every orphan. */
    for(unsigned i=0;i<128;++i){
        fd=open(path,O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fd>=0);
        CHECK(write(fd,"x",1)==1&&!unlink(path)&&!close(fd));
    }
    CHECK(!mkdir("/unlink-dir",0700));
    fd=open("/unlink-dir/file",O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fd>=0);
    CHECK(write(fd,"kept",4)==4&&!unlink("/unlink-dir/file"));
    CHECK(!unlinkat(AT_FDCWD,"/unlink-dir",AT_REMOVEDIR));
    CHECK(!mkdir("/unlink-dir",0700));
    fresh=open("/unlink-dir/file",O_CREAT|O_EXCL|O_RDWR,0600);CHECK(fresh>=0);
    CHECK(lseek(fd,0,SEEK_SET)==0&&read(fd,data,4)==4&&!memcmp(data,"kept",4));
    CHECK(!close(fd)&&!close(fresh)&&!unlink("/unlink-dir/file"));
    CHECK(!unlinkat(AT_FDCWD,"/unlink-dir",AT_REMOVEDIR));
    char bad[]="/temp-invalid";
    CHECK(mkstemp(bad)==-1&&errno==EINVAL&&!strcmp(bad,"/temp-invalid"));
    char a[]="/temp-XXXXXX",b[]="/temp-XXXXXX";
    fd=mkstemp(a);fresh=mkstemp(b);CHECK(fd>=0&&fresh>=0&&strcmp(a,b));
    CHECK(!unlink(a)&&!unlink(b)&&!ftruncate(fd,8192));
    CHECK(!fstat(fd,&st)&&st.st_size==8192);
    CHECK(read(fd,data,sizeof(data))==sizeof(data));
    for(unsigned i=0;i<sizeof(data);++i)CHECK(!data[i]);
    CHECK(!close(fd)&&!close(fresh));
    char directory[]="/temp-dir-XXXXXX";CHECK(mkdtemp(directory)==directory);
    CHECK(!stat(directory,&st)&&S_ISDIR(st.st_mode));
    CHECK(!unlinkat(AT_FDCWD,directory,AT_REMOVEDIR));
    CHECK(!setenv("XDG_RUNTIME_DIR","/",1));
    for(unsigned i=0;i<128;++i){
        fd=os_create_anonymous_file(8192,"myos");CHECK(fd>=0);
        CHECK(fcntl(fd,F_GETFD)==FD_CLOEXEC&&!fstat(fd,&st)&&st.st_size==8192);
        CHECK(read(fd,data,sizeof(data))==sizeof(data));
        for(unsigned j=0;j<sizeof(data);++j)CHECK(!data[j]);
        CHECK(!close(fd));
    }
    CHECK(os_create_anonymous_file(-1,0)==-1&&errno==EINVAL);
    CHECK(os_create_anonymous_file(65537,0)==-1&&errno==27);
    puts("native unlink open/dup/name reuse/last close PASS");return 84;
}
