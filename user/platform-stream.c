/* Owned unbuffered target streams. No host FILE, syscalls or thread state. */
#include "stdio_impl.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#undef ferror
static size_t descriptor_write(FILE *,const unsigned char *,size_t);
static unsigned char out_sentinel[1],err_sentinel[1],in_sentinel[1];
static FILE standard_out={.flags=F_STD|F_NORD,.buf=out_sentinel,.write=descriptor_write,.lbf=-1,.fd=1};
static FILE standard_err={.flags=F_STD|F_NORD,.buf=err_sentinel,.write=descriptor_write,.lbf=-1,.fd=2};
static FILE standard_in={.flags=F_STD|F_NOWR,.buf=in_sentinel,.write=descriptor_write,.lbf=-1,.fd=0};
FILE *const stdout=&standard_out;
FILE *const stderr=&standard_err;
FILE *const stdin=&standard_in;
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static FILE *opened;
struct memory_stream {
    char **output;
    size_t *size;
    unsigned char *data;
    size_t position,length,capacity;
};
static void acquire(pthread_mutex_t *m){if(pthread_mutex_lock(m))__builtin_trap();}
static void release(pthread_mutex_t *m){if(pthread_mutex_unlock(m))__builtin_trap();}
static int error(int code){errno=code;return EOF;}
static ssize_t console_write(const unsigned char *data,size_t length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)data;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
    if((int64_t)a<0)return error((int)-(int64_t)a);
    return (ssize_t)a;
}
static size_t transfer(FILE *f,const unsigned char *data,size_t length)
{
    size_t done=0;
    while(done<length){
        size_t amount=length-done;if((f->flags&F_STD)&&amount>256)amount=256;
        ssize_t result=(f->flags&F_STD)?console_write(data+done,amount):write(f->fd,data+done,amount);
        if(result<0){if(errno==EINTR)continue;break;}
        if(!result){errno=EIO;break;}done+=(size_t)result;
    }
    if(done<length)f->flags|=F_ERR;
    return done;
}
static size_t descriptor_write(FILE *f,const unsigned char *data,size_t length)
{
    if(f->fd<0||(f->flags&F_NOWR)){f->flags|=F_ERR;errno=EBADF;f->wpos=f->wbase=f->wend=0;return 0;}
    size_t pending=f->wpos?(size_t)(f->wpos-f->wbase):0;
    if(transfer(f,f->wbase,pending)!=pending){f->wpos=f->wbase=f->wend=0;return 0;}
    size_t done=transfer(f,data,length);
    if(done!=length)f->wpos=f->wbase=f->wend=0;
    else {f->wpos=f->wbase=f->buf;f->wend=f->buf+f->buf_size;}
    return done;
}
static int mode_flags(const char *mode,int *flags,unsigned *access)
{
    if(!mode||!*mode)return error(EINVAL);
    switch(*mode){
    case 'r':*flags=O_RDONLY;*access=1;break;
    case 'w':*flags=O_WRONLY|O_CREAT|O_TRUNC;*access=2;break;
    case 'a':*flags=O_WRONLY|O_CREAT|O_APPEND;*access=2;break;
    default:return error(EINVAL);
    }
    unsigned seen=0;for(const char *p=mode+1;*p;++p){unsigned bit;
        if(*p=='+'){bit=1;*flags=(*flags&~O_ACCMODE)|O_RDWR;*access=3;}
        else if(*p=='b')bit=2;
        else if(*p=='e'){bit=4;*flags|=O_CLOEXEC;}
        else if(*p=='x'&&*mode=='w'){bit=8;*flags|=O_EXCL;}
        else return error(EINVAL);
        if(seen&bit)return error(EINVAL);
        seen|=bit;
    }
    return 0;
}
static FILE *adopt(int fd,unsigned access,int publish)
{
    FILE *f=malloc(sizeof(*f));if(!f)return 0;
    *f=(FILE){.flags=(access&1?0:F_NORD)|(access&2?0:F_NOWR),.write=descriptor_write,.lbf=-1,.fd=fd};
    f->buf=f->sentinel;
    int result=pthread_mutex_init(&f->mutex,0);
    if(result){free(f);errno=result;return 0;}
    if(publish){acquire(&registry);f->next=opened;opened=f;release(&registry);}
    return f;
}
static size_t memory_write(FILE *f,const unsigned char *data,size_t length)
{
    struct memory_stream *m=f->cookie;
    size_t pending=f->wpos?(size_t)(f->wpos-f->wbase):0;
    if(pending>INT64_MAX-m->position||length>INT64_MAX-m->position-pending){
        errno=EOVERFLOW;goto fail;
    }
    size_t total=pending+length,end=m->position+total;
    if(total&&end>=m->capacity){
        size_t capacity=m->capacity;
        while(capacity<=end){
            if(capacity>SIZE_MAX/2){capacity=end+1;break;}
            capacity*=2;
        }
        unsigned char *grown=realloc(m->data,capacity);
        if(!grown)goto fail;
        memset(grown+m->capacity,0,capacity-m->capacity);
        m->data=grown;m->capacity=capacity;
    }
    if(pending)memcpy(m->data+m->position,f->wbase,pending);
    if(length)memcpy(m->data+m->position+pending,data,length);
    m->position=end;
    if(total&&end>m->length)m->length=end;
    m->data[m->length]=0;
    *m->output=(char *)m->data;
    *m->size=m->position<m->length?m->position:m->length;
    f->wpos=f->wbase=f->buf;f->wend=f->buf+f->buf_size;
    return length;
fail:
    f->flags|=F_ERR;f->wpos=f->wbase=f->wend=0;return 0;
}
FILE *open_memstream(char **output,size_t *size)
{
    if(!output||!size){errno=EINVAL;return 0;}
    struct memory_stream *m=malloc(sizeof(*m));if(!m)return 0;
    unsigned char *data=malloc(1);if(!data){free(m);return 0;}
    *data=0;
    *m=(struct memory_stream){.output=output,.size=size,.data=data,.capacity=1};
    FILE *f=adopt(-1,2,0);
    if(!f){int saved=errno;free(data);free(m);errno=saved;return 0;}
    acquire(&f->mutex);f->cookie=m;f->write=memory_write;f->flags|=F_MEM;
    *output=(char *)data;*size=0;release(&f->mutex);
    acquire(&registry);f->next=opened;opened=f;release(&registry);return f;
}
FILE *fopen(const char *path,const char *mode)
{
    int flags;unsigned access;if(mode_flags(mode,&flags,&access))return 0;
    int fd=openat(AT_FDCWD,path,flags,0600);if(fd<0)return 0;
    FILE *f=adopt(fd,access,1);if(!f){int saved=errno;close(fd);errno=saved;}return f;
}
FILE *fdopen(int fd,const char *mode)
{
    int flags;unsigned access;if(mode_flags(mode,&flags,&access))return 0;
    if(flags&O_EXCL){errno=EINVAL;return 0;}
    int actual=fcntl(fd,F_GETFL);if(actual<0)return 0;
    int fdflags=fcntl(fd,F_GETFD);if(fdflags<0)return 0;
    unsigned permitted=(actual&O_ACCMODE)==O_RDONLY?1:(actual&O_ACCMODE)==O_WRONLY?2:3;
    if(access&~permitted){errno=EINVAL;return 0;}
    FILE *f=adopt(fd,access,1);if(!f)return 0;
    if((flags&O_CLOEXEC)&&fcntl(fd,F_SETFD,(long)(fdflags|FD_CLOEXEC))<0)goto rollback;
    if((flags&O_APPEND)&&fcntl(fd,F_SETFL,(long)(actual|O_APPEND))<0){
        int saved=errno;
        if(flags&O_CLOEXEC)fcntl(fd,F_SETFD,(long)fdflags);
        errno=saved;goto rollback;
    }
    return f;
rollback: {
        /* Adoption rollback must not close the caller-owned descriptor. */
        int saved=errno;acquire(&registry);FILE **p=&opened;while(*p!=f)p=&(*p)->next;*p=f->next;release(&registry);
        pthread_mutex_destroy(&f->mutex);free(f);errno=saved;return 0;
    }
}
static int flush_locked(FILE *f)
{
    if(f->flags&F_MEM){f->write(f,0,0);return f->wpos?0:EOF;}
    if(f->fd<0)return error(EBADF);
    if(f->wpos&&f->wpos!=f->wbase){f->write(f,0,0);if(!f->wpos)return EOF;}
    return 0;
}
int fflush(FILE *f)
{
    if(f){acquire(&f->mutex);int result=flush_locked(f);release(&f->mutex);return result;}
    int result=0;acquire(&registry);
    FILE *standard[]={stdout,stderr};
    for(unsigned i=0;i<2;++i){acquire(&standard[i]->mutex);
        if(standard[i]->fd>=0&&flush_locked(standard[i]))result=EOF;
        release(&standard[i]->mutex);}
    for(FILE *p=opened;p;p=p->next){acquire(&p->mutex);if(flush_locked(p))result=EOF;release(&p->mutex);}
    release(&registry);return result;
}
int fclose(FILE *f)
{
    if(!f)return error(EINVAL);
    acquire(&registry);acquire(&f->mutex);int result=flush_locked(f);
    if(f->flags&F_STD){f->fd=-1;f->flags|=F_NOWR|F_NORD;release(&f->mutex);release(&registry);return result;}
    FILE **p=&opened;while(*p&&*p!=f)p=&(*p)->next;
    if(!*p){release(&f->mutex);release(&registry);return error(EINVAL);}
    *p=f->next;
    if(f->flags&F_MEM)free(f->cookie); /* Published data belongs to the caller. */
    else if(close(f->fd))result=EOF;
    release(&f->mutex);release(&registry);pthread_mutex_destroy(&f->mutex);free(f);return result;
}
size_t fread(void *data,size_t size,size_t count,FILE *f)
{
    if(size&&count>SIZE_MAX/size){errno=EOVERFLOW;return 0;}
    if(!size||!count)return 0;
    acquire(&f->mutex);size_t done=0,length=size*count;
    if((f->flags&F_NORD)||(!(f->flags&F_MEM)&&f->fd<0)){f->flags|=F_ERR;errno=EBADF;}
    else if(f->flags&F_STD){f->flags|=F_ERR;errno=ENOTSUP;}
    else if(!(f->flags&F_EOF))while(done<length){
        ssize_t got=read(f->fd,(unsigned char *)data+done,length-done);
        if(got<0){if(errno==EINTR)continue;f->flags|=F_ERR;break;}
        if(!got){f->flags|=F_EOF;break;}done+=(size_t)got;
    }
    release(&f->mutex);return done/size;
}
int fseek(FILE *f,long offset,int whence)
{
    acquire(&f->mutex);int result=flush_locked(f);
    if(!result){if(f->flags&F_MEM){
            struct memory_stream *m=f->cookie;
            if(whence<SEEK_SET||whence>SEEK_END)result=error(EINVAL);
            else {
                size_t base=whence==SEEK_SET?0:whence==SEEK_CUR?m->position:m->length;
                if(offset<0){uint64_t amount=0-(uint64_t)offset;
                    if(amount>base)result=error(EINVAL);else m->position=base-amount;
                }else if((uint64_t)offset>INT64_MAX-base)result=error(EOVERFLOW);
                else m->position=base+(uint64_t)offset;
            }
        }else if(f->flags&F_STD)result=error(ESPIPE);
        else if(lseek(f->fd,offset,whence)<0)result=EOF;
        else {f->flags&=~F_EOF;f->rpos=f->rend=0;}}
    release(&f->mutex);return result;
}
long ftell(FILE *f)
{acquire(&f->mutex);long result=(f->flags&F_MEM)?(long)((struct memory_stream *)f->cookie)->position:
    (f->flags&F_STD)?error(ESPIPE):lseek(f->fd,0,SEEK_CUR);release(&f->mutex);return result;}
int fileno(FILE *f){acquire(&f->mutex);int fd=f->fd;release(&f->mutex);return fd<0?error(EBADF):fd;}
int feof(FILE *f){acquire(&f->mutex);int result=!!(f->flags&F_EOF);release(&f->mutex);return result;}
int ferror(FILE *f){acquire(&f->mutex);int result=!!(f->flags&F_ERR);release(&f->mutex);return result;}
void clearerr(FILE *f){acquire(&f->mutex);f->flags&=~(F_ERR|F_EOF);release(&f->mutex);}
int setvbuf(FILE *f,char *buffer,int mode,size_t size)
{(void)buffer;(void)size;if(mode!=_IONBF)return error(ENOTSUP);acquire(&f->mutex);int result=flush_locked(f);release(&f->mutex);return result;}
int fputc(int value,FILE *f){unsigned char byte=(unsigned char)value;return fwrite(&byte,1,1,f)==1?byte:EOF;}
int fgetc(FILE *f){unsigned char byte;return fread(&byte,1,1,f)==1?byte:EOF;}
int fputs(const char *text,FILE *f){size_t size=strlen(text);return fwrite(text,1,size,f)==size?0:EOF;}
int puts(const char *text)
{
    size_t size=strlen(text);acquire(&stdout->mutex);
    int result=__fwritex((const unsigned char *)text,size,stdout)==size&&
        __fwritex((const unsigned char *)"\n",1,stdout)==1?0:EOF;
    release(&stdout->mutex);return result;
}
int fprintf(FILE *f,const char *format,...)
{va_list args;va_start(args,format);int result=vfprintf(f,format,args);va_end(args);return result;}
int printf(const char *format,...)
{va_list args;va_start(args,format);int result=vfprintf(stdout,format,args);va_end(args);return result;}
int vprintf(const char *format,va_list args){return vfprintf(stdout,format,args);}
