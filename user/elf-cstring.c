#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif
void *mmap(void *,size_t,int,int,int,long);
int munmap(void *,size_t);
#ifdef __cplusplus
}
#endif
static void require(int good) {
    if(!good) { const char fail[]="native Cstring assertion failed\n";
        write(1,fail,sizeof(fail)-1);abort(); }
}
static int compare(const char *a,const char *b) {
#ifdef __cplusplus
    return std::strcoll(a,b);
#else
    return strcoll(a,b);
#endif
}
static size_t transform(char *out,const char *in,size_t n) {
#ifdef __cplusplus
    return std::strxfrm(out,in,n);
#else
    return strxfrm(out,in,n);
#endif
}
static int sign(int n) { return (n>0)-(n<0); }
static void collation(void) {
    /* Explicit fixed bytewise profile: no claim of linguistic ordering. */
    const char *strings[]={"","A","AA","B","a","\x7f","\x80",
        "\xe0\xb8\x81","\xf0\x9f\x98\x80","\xff"};
    char transformed[10][16];
    for(int i=0;i<10;++i) {
        size_t len=strlen(strings[i]);
        errno=729;require(transform(0,strings[i],0)==len && errno==729);
        require(transform(transformed[i],strings[i],16)==len);
        require(!memcmp(transformed[i],strings[i],len+1));
        for(size_t n=0;n<=len+2;++n) {
            unsigned char bounded[32];memset(bounded,0x5a,sizeof(bounded));
            errno=730;require(transform((char *)bounded+1,strings[i],n)==len && errno==730);
            require(bounded[0]==0x5a);
            for(size_t k=n+1;k<sizeof(bounded);++k)require(bounded[k]==0x5a);
            /* For n<=len the result bytes are unspecified; only bounds matter. */
            if(n>len)require(!memcmp(bounded+1,strings[i],len+1));
        }
    }
    for(int i=0;i<10;++i)for(int j=0;j<10;++j) {
        errno=731;int result=compare(strings[i],strings[j]);
        require(sign(result)==sign(i-j) && errno==731);
        require(sign(result)==sign(strcmp(transformed[i],transformed[j])));
    }
}
static void guards(void) {
    char *source=(char *)mmap(0,8192,3,0x22,-1,0);
    char *dest=(char *)mmap(0,8192,3,0x22,-1,0);
    require(source!=(char *)-1 && dest!=(char *)-1);
    require(munmap(source+4096,4096)==0 && munmap(dest+4096,4096)==0);
    /* Exercise each source alignment and exact end-of-page destination. */
    for(size_t len=0;len<32;++len) {
        char *s=source+4095-len;
        for(size_t k=0;k<len;++k)s[k]=(char)('a'+k%26);
        s[len]=0;
        require(transform(0,s,0)==len && compare(s,s)==0);
        char *d=dest+4095-len;
        require(transform(d,s,len+1)==len && !memcmp(d,s,len+1));
        for(size_t n=0;n<=len;++n)require(transform(dest+4096-n,s,n)==len);
    }
    require(munmap(source,4096)==0 && munmap(dest,4096)==0);
}
static void *worker(void *value) { for(int i=0;i<20;++i)collation();return value; }
#ifdef __cplusplus
extern "C"
#endif
int _start(void) {
    collation();guards();pthread_t threads[8];
    for(unsigned long i=0;i<8;++i)require(pthread_create(&threads[i],0,worker,(void *)(i+1))==0);
    for(unsigned long i=0;i<8;++i) { void *value=0;
        require(pthread_join(threads[i],&value)==0 && value==(void *)(i+1)); }
    const char pass[]="native Cstring bytewise/transform/guards/8workers PASS\n";
    require(write(1,pass,sizeof(pass)-1)==(ssize_t)(sizeof(pass)-1));return 99;
}
