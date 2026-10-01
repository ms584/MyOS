#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include "memstream.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"memstream failed: %s\n",#x);__builtin_trap();}}while(0)
static void *held[20000];
static FILE *shared;
static void *worker(void *arg)
{
    for(int i=0;i<16;++i)CHECK(fprintf(shared,"%ld:%02d\n",(long)(uintptr_t)arg,i)==5);
    return arg;
}
int _start(void)
{
    char *data=(void *)1;size_t size=99;
    CHECK(!open_memstream(0,&size)&&errno==EINVAL&&size==99);
    CHECK(!open_memstream(&data,0)&&errno==EINVAL&&data==(void *)1);
    FILE *f=open_memstream(&data,&size);CHECK(f&&data&&size==0&&!*data);
    CHECK(fileno(f)==-1&&errno==EBADF);
    CHECK(fgetc(f)==EOF&&ferror(f)&&errno==EBADF);clearerr(f);
    CHECK(fprintf(f,"hello my world")==14&&!fflush(f));
    CHECK(size==14&&!strcmp(data,"hello my world")&&ftell(f)==14);
    CHECK(!fseek(f,0,SEEK_SET)&&fprintf(f,"good-bye")==8);
    CHECK(!fflush(f)&&size==8&&data[14]==0);
    CHECK(!fseek(f,14,SEEK_SET)&&!fclose(f));
    CHECK(size==14&&!strcmp(data,"good-bye world"));free(data);
    f=open_memstream(&data,&size);CHECK(f);
    CHECK(fwrite("A\0B",1,3,f)==3&&!fflush(0)&&size==3&&!data[3]);
    CHECK(!fseek(f,10,SEEK_SET)&&!fflush(f)&&size==3&&ftell(f)==10);
    CHECK(fputc('X',f)=='X'&&!fflush(f)&&size==11);
    for(unsigned i=3;i<10;++i)CHECK(!data[i]);
    CHECK(data[0]=='A'&&!data[1]&&data[2]=='B'&&data[10]=='X'&&!data[11]);
    CHECK(!fseek(f,-1,SEEK_END)&&ftell(f)==10);
    CHECK(fseek(f,-11,SEEK_CUR)==-1&&errno==EINVAL&&ftell(f)==10);
    CHECK(fseek(f,0,3)==-1&&errno==EINVAL&&ftell(f)==10);
    CHECK(!fseek(f,INT64_MAX,SEEK_SET)&&!fflush(f)&&size==11);
    char *preserved=data;CHECK(fputc('Y',f)==EOF&&errno==EOVERFLOW&&ferror(f));
    CHECK(data==preserved&&size==11);clearerr(f);
    CHECK(!fseek(f,11,SEEK_SET)&&fputc('Y',f)=='Y'&&!fclose(f));
    CHECK(size==12&&data[11]=='Y'&&!data[12]);free(data);
    f=open_memstream(&data,&size);CHECK(f&&fputs("seed",f)>=0&&!fflush(f));
    preserved=data;size_t n=0;
    while(n<20000&&(held[n]=malloc(131072)))++n;
    while(n<20000&&(held[n]=malloc(1)))++n;
    CHECK(n<20000);
    char payload[128];memset(payload,'P',sizeof(payload));
    CHECK(fwrite(payload,1,sizeof(payload),f)==0&&errno==ENOMEM&&ferror(f));
    CHECK(data==preserved&&size==4&&!strcmp(data,"seed")&&ftell(f)==4);
    char *other=(void *)1;size_t other_size=77;
    CHECK(!open_memstream(&other,&other_size)&&errno==ENOMEM&&other==(void *)1&&other_size==77);
    while(n)free(held[--n]);
    clearerr(f);CHECK(fwrite(payload,1,sizeof(payload),f)==sizeof(payload));
    CHECK(!fclose(f)&&size==132&&!data[132]);free(data);
    shared=open_memstream(&data,&size);CHECK(shared);
    pthread_t threads[8];
    for(uintptr_t i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)i));
    for(uintptr_t i=0;i<8;++i){void *result;CHECK(!pthread_join(threads[i],&result)&&result==(void *)i);}
    CHECK(!fflush(0)&&size==640&&ftell(shared)==640);
    unsigned seen[8]={0};
    for(unsigned i=0;i<640;i+=5){
        unsigned id=data[i]-'0',value=(data[i+2]-'0')*10+data[i+3]-'0';
        CHECK(id<8&&value<16&&data[i+1]==':'&&data[i+4]=='\n');
        CHECK(!(seen[id]&(1u<<value)));seen[id]|=1u<<value;
    }
    for(unsigned i=0;i<8;++i)CHECK(seen[i]==65535);
    CHECK(!fclose(shared)&&size==640&&!data[640]);free(data);
    struct u_memstream mesa;
    CHECK(u_memstream_open(&mesa,&data,&size));
    CHECK(fprintf(u_memstream_get(&mesa),"Mesa %s:%d","native",27)==14);
    CHECK(!u_memstream_flush(&mesa)&&size==14&&!strcmp(data,"Mesa native:27"));
    u_memstream_close(&mesa);CHECK(size==14&&!strcmp(data,"Mesa native:27"));free(data);
    CHECK(puts("native memory stream/real Mesa/seek/ownership/OOM/threads PASS")>=0);
    return 81;
}
