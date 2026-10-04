#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"stream failed: %s\n",#x);__builtin_trap();}}while(0)
static FILE *shared;
static pthread_mutex_t seen_lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned line_seen[8];
static void *line_reader(void *arg)
{
    char line[6];
    while(fgets(line,sizeof(line),shared)){
        unsigned id=line[0]-'0',value=(line[2]-'0')*10+line[3]-'0';
        CHECK(id<8&&value<16&&line[1]==':'&&line[4]=='\n'&&!line[5]);
        CHECK(!pthread_mutex_lock(&seen_lock));
        CHECK(!(line_seen[id]&(1u<<value)));line_seen[id]|=1u<<value;
        CHECK(!pthread_mutex_unlock(&seen_lock));
    }
    CHECK(feof(shared)&&!ferror(shared));return arg;
}
static void *held[20000];
#ifdef STREAM_CANCEL
static FILE *blocked;
static void *reader(void *arg)
{(void)arg;char line[16];puts("stream blocked reader ready");fgets(line,sizeof(line),blocked);__builtin_trap();}
#endif
static void *worker(void *arg)
{
    for(int i=0;i<16;++i)CHECK(fprintf(shared,"%ld:%02d\n",(long)(uintptr_t)arg,i)==5);
    return arg;
}
#ifdef __cplusplus
extern "C"
#endif
int _start(void)
{
#ifdef STREAM_CANCEL
    char *cancel_buffer;size_t cancel_size;
    FILE *cancel_memory=open_memstream(&cancel_buffer,&cancel_size);CHECK(cancel_memory);
    char block[2048];memset(block,'M',sizeof(block));
    for(unsigned i=0;i<32;++i)CHECK(fwrite(block,1,sizeof(block),cancel_memory)==sizeof(block));
    CHECK(!fflush(cancel_memory)&&cancel_size==65536);
    int ends[2];CHECK(!pipe(ends));blocked=fdopen(ends[0],"r");CHECK(blocked);
    /* Existing asset avoids conflating persistent RAMFS data with leaked handles. */
    shared=fopen("/assets/stream","r");CHECK(shared&&fgetc(shared)>=0);
    pthread_t thread;CHECK(!pthread_create(&thread,0,reader,0));
    void *result;pthread_join(thread,&result);__builtin_trap();
#endif
    const char *path="/stream-sdk";
    unlinkat(AT_FDCWD,path,0);
    CHECK(!fopen(path,"q")&&errno==EINVAL);
    FILE *f=fopen(path,"w+e");CHECK(f);
    int fd=fileno(f);CHECK(fd>=0&&fcntl(fd,F_GETFD)==FD_CLOEXEC);
    CHECK(!setvbuf(f,0,_IONBF,0));
    errno=0;CHECK(setvbuf(f,0,_IOLBF,0)==-1&&errno==ENOTSUP&&ftell(f)==0);
    errno=0;CHECK(setvbuf(f,0,_IOFBF,0)==-1&&errno==ENOTSUP&&ftell(f)==0);
    CHECK(fwrite("a\0bcde",2,3,f)==3&&ftell(f)==6);
    CHECK(fprintf(f,"-%d",17)==3&&ftell(f)==9);
    CHECK(!fflush(0));CHECK(!fseek(f,0,SEEK_SET));
    char data[1024];CHECK(fread(data,2,5,f)==4&&feof(f)&&!ferror(f));
    data[9]=0;CHECK(data[0]=='a'&&!data[1]&&data[2]=='b'&&data[5]=='e');
    CHECK(!strcmp(data+6,"-17"));
    CHECK(!fseek(f,0,SEEK_SET)&&!feof(f));
    CHECK(fgetc(f)=='a'&&fgetc(f)==0);
    CHECK(!fclose(f)&&fcntl(fd,F_GETFL)==-1&&errno==EBADF);
    CHECK(!fopen(path,"wx")&&errno==17);
    f=fopen(path,"a");CHECK(f&&!fseek(f,0,SEEK_SET));
    CHECK(fputs("!",f)>=0&&!fclose(f));
    f=fopen(path,"r");CHECK(f&&ftell(f)==0);
    errno=0;CHECK(fprintf(f,"bad")==-1&&ferror(f)&&errno==EBADF);
    clearerr(f);CHECK(!ferror(f));
    CHECK(fread(data,SIZE_MAX,2,f)==0&&errno==EOVERFLOW&&ftell(f)==0);
    CHECK(fread(data,1,0,f)==0&&ftell(f)==0);
    CHECK(fread(data,1,11,f)==10&&feof(f));
    CHECK(data[9]=='!'&&!fclose(f));
    fd=openat(AT_FDCWD,path,O_RDONLY);
    CHECK(fd>=0&&!fdopen(fd,"w")&&errno==EINVAL&&fcntl(fd,F_GETFL)>=0);
    CHECK(!fdopen(fd,"wx")&&errno==EINVAL);
    CHECK(!close(fd));
    fd=openat(AT_FDCWD,path,O_RDWR);
    f=fdopen(fd,"ae");CHECK(f);
    CHECK((fcntl(fd,F_GETFL)&O_APPEND)&&fcntl(fd,F_GETFD)==FD_CLOEXEC);
    CHECK(!fseek(f,0,SEEK_SET)&&fputc('?',f)=='?'&&!fclose(f));
    CHECK(fcntl(fd,F_GETFD)==-1&&errno==EBADF);
    shared=fopen(path,"w+");CHECK(shared);
    pthread_t threads[8];
    for(uintptr_t i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)i));
    for(uintptr_t i=0;i<8;++i){void *result;CHECK(!pthread_join(threads[i],&result)&&result==(void *)i);}
    CHECK(ftell(shared)==640&&!fseek(shared,0,SEEK_SET));
    CHECK(fread(data,1,640,shared)==640);
    unsigned seen[8]={0};
    for(unsigned i=0;i<640;i+=5){
        unsigned id=data[i]-'0',value=(data[i+2]-'0')*10+data[i+3]-'0';
        CHECK(id<8&&value<16&&data[i+1]==':'&&data[i+4]=='\n');
        CHECK(!(seen[id]&(1u<<value)));seen[id]|=1u<<value;
    }
    for(unsigned i=0;i<8;++i)CHECK(seen[i]==65535);
    CHECK(!fclose(shared));
    shared=fopen(path,"r");CHECK(shared);
    for(uintptr_t i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,line_reader,(void *)i));
    for(uintptr_t i=0;i<8;++i){void *result;CHECK(!pthread_join(threads[i],&result)&&result==(void *)i);}
    for(unsigned i=0;i<8;++i)CHECK(line_seen[i]==65535);
    CHECK(!fclose(shared)&&!pthread_mutex_destroy(&seen_lock));
    f=fopen(path,"w+");CHECK(f);
    CHECK(fwrite("\xff\0Z\nabcdef\nlast",1,15,f)==15&&!fseek(f,0,SEEK_SET));
    CHECK(getc(f)==255&&getc(f)==0&&!fseek(f,0,SEEK_SET));
    unsigned char guard[10];memset(guard,0xa5,sizeof(guard));
    CHECK(!fgets((char *)guard+1,0,f)&&errno==EINVAL&&guard[1]==0xa5&&ftell(f)==0);
    CHECK(!fgets((char *)guard+1,-1,f)&&errno==EINVAL&&ftell(f)==0);
    CHECK(fgets((char *)guard+1,1,f)==(char *)guard+1&&!guard[1]&&ftell(f)==0);
    CHECK(fgets((char *)guard+1,5,f)==(char *)guard+1);
    CHECK(guard[0]==0xa5&&guard[1]==255&&!guard[2]&&guard[3]=='Z'&&guard[4]=='\n'&&!guard[5]&&guard[6]==0xa5);
    CHECK(fgets(data,4,f)==data&&!strcmp(data,"abc"));
    CHECK(fgets(data,8,f)==data&&!strcmp(data,"def\n"));
    CHECK(fgets(data,8,f)==data&&!strcmp(data,"last")&&feof(f));
    data[0]='Q';CHECK(!fgets(data,8,f)&&data[0]=='Q');
    CHECK(!fseek(f,0,SEEK_SET)&&!feof(f)&&getc(f)==255);
    CHECK(!fclose(f));f=fopen(path,"w");CHECK(f);
    CHECK(!fgets(data,8,f)&&ferror(f)&&errno==EBADF);CHECK(!fclose(f));
    f=fopen(path,"r");CHECK(f);data[0]='Q';
    CHECK(!fgets(data,8,f)&&feof(f)&&!ferror(f)&&data[0]=='Q');CHECK(!fclose(f));
    int pipe_ends[2];CHECK(!pipe2(pipe_ends,O_NONBLOCK));
    f=fdopen(pipe_ends[0],"r");CHECK(f);
    CHECK(write(pipe_ends[1],"AB",2)==2);
    memset(guard,0xa5,sizeof(guard));
    CHECK(!fgets((char *)guard+1,5,f)&&errno==EAGAIN&&ferror(f)&&!feof(f));
    CHECK(guard[0]==0xa5&&guard[1]=='A'&&guard[2]=='B'&&guard[6]==0xa5);
    clearerr(f);CHECK(!close(pipe_ends[1]));
    CHECK(!fgets(data,8,f)&&feof(f)&&!ferror(f));CHECK(!fclose(f));
    FILE *files[8];for(unsigned i=0;i<8;++i){files[i]=fopen(path,"r");CHECK(files[i]);}
    CHECK(!fopen(path,"r")&&errno==24);
    for(unsigned i=0;i<8;++i)CHECK(!fclose(files[i]));
    size_t n=0;
    while(n<20000&&(held[n]=malloc(131072)))++n;
    while(n<20000&&(held[n]=malloc(1)))++n;
    CHECK(n<20000);
    CHECK(!fopen(path,"r")&&errno==ENOMEM);
    while(n)free(held[--n]);
    /* fopen allocation failure must release its freshly opened descriptor. */
    for(unsigned i=0;i<8;++i){files[i]=fopen(path,"r");CHECK(files[i]);}
    for(unsigned i=0;i<8;++i)CHECK(!fclose(files[i]));
    CHECK(!unlinkat(AT_FDCWD,path,0));
    CHECK(fseek(stdout,0,SEEK_SET)==-1&&errno==ESPIPE);
    CHECK(fgetc(stdin)==EOF&&ferror(stdin)&&errno==ENOTSUP);clearerr(stdin);
    char longtext[601];for(unsigned i=0;i<600;++i)longtext[i]='Z';longtext[600]=0;
    CHECK(printf("stream-console:%s:end\n",longtext)==620);
    CHECK(fprintf(stderr,"stream-stderr:%d\n",123)==18);
    CHECK(puts("native stream file/console/ownership/thread/limit PASS")>=0);
    return 80;
}
