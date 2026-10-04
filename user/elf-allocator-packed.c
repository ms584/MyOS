/* Native allocator regression, not a Cobalt acceptance fixture. */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
extern size_t malloc_usable_size(void *);
extern void *mmap(void *,size_t,int,int,int,int64_t);
extern int munmap(void *,size_t);
#define VM_BASE UINT64_C(0x8020000000)
#define VM_END UINT64_C(0x803f000000)
static void *held[4096];
static void say(const char *s){write(1,s,strlen(s));}
#define CHECK(x) do{if(!(x)){say("Packed allocator FAIL: " #x "\n");__builtin_trap();}}while(0)
static void *worker(void *arg)
{
    unsigned id=(unsigned)(uintptr_t)arg;
    const size_t sizes[]={1,31,32,33,95,96,223,480,991,992};
    errno=170+(int)id;
    for(unsigned round=0;round<300;++round){
        size_t n=sizes[(round+id)%10];unsigned char value=(unsigned char)(round+id+1);
        unsigned char *p=malloc(n);CHECK(p&&!((uintptr_t)p&15));
        CHECK(malloc_usable_size(p)==n);memset(p,value,n);
        unsigned char *zero=calloc(1,n);CHECK(zero);
        for(size_t i=0;i<n;++i)CHECK(!zero[i]&&p[i]==value);
        free(zero);p=realloc(p,1025);CHECK(p&&malloc_usable_size(p)==1025);
        for(size_t i=0;i<n;++i)CHECK(p[i]==value);
        p=realloc(p,1);CHECK(p&&p[0]==value);free(p);
    }
    CHECK(errno==170+(int)id);return (void *)(uintptr_t)(id+1);
}
int _start(void)
{
    /* White-box private-state boundary/wrap checks, NOT public API promises. */
    uintptr_t *cursor=(void *)(uintptr_t)UINT64_C(0x803f0106d0);
    CHECK(!*cursor);
    uint8_t guards[64];memcpy(guards,(void *)(uintptr_t)UINT64_C(0x803f010690),64);
    unsigned char *collision=mmap((void *)(uintptr_t)VM_BASE,4096,3,0x100022,-1,0);
    CHECK(collision==(void *)(uintptr_t)VM_BASE);memset(collision,0x5a,4096);
    errno=41;unsigned char *p=malloc(17);CHECK(p&&errno==41);
    CHECK((uintptr_t)p>=VM_BASE+4096);free(p);
    *cursor=VM_END-4096;p=malloc(16384);CHECK(p&&errno==41);
    for(unsigned i=0;i<4096;++i)CHECK(collision[i]==0x5a);
    free(p);CHECK(!munmap(collision,4096));
    long before=sysconf(_SC_AVPHYS_PAGES);CHECK(before>0);
    for(unsigned i=0;i<512;++i){held[i]=malloc(96);CHECK(held[i]);memset(held[i],0xa5,96);}
    long after=sysconf(_SC_AVPHYS_PAGES);CHECK(before-after>0&&before-after<64);
    for(unsigned i=0;i<512;i+=2){free(held[i]);held[i]=0;}
    for(unsigned i=0;i<256;++i){p=calloc(1,96);CHECK(p);
        for(unsigned j=0;j<96;++j){CHECK(!p[j]);}
        free(p);}
    for(unsigned i=1;i<512;i+=2){p=held[i];
        for(unsigned j=0;j<96;++j){CHECK(p[j]==0xa5);}
        free(p);}
    CHECK(!memcmp(guards,(void *)(uintptr_t)UINT64_C(0x803f010690),64));
    CHECK(!malloc(SIZE_MAX)&&errno==ENOMEM);
    CHECK(!calloc(SIZE_MAX,2)&&errno==ENOMEM);
    CHECK(!aligned_alloc(64,65)&&errno==EINVAL);
    void *aligned=(void *)(uintptr_t)0x1234;
    CHECK(posix_memalign(&aligned,3,1)==EINVAL&&aligned==(void *)(uintptr_t)0x1234);
    for(size_t a=8;a<=8192;a*=2){p=aligned_alloc(a,a*2);CHECK(p&&!((uintptr_t)p&(a-1)));
        memset(p,0x6b,a*2);free(p);}
    pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)i));
    for(unsigned i=0;i<8;++i){void *result;
        CHECK(!pthread_join(threads[i],&result)&&result==(void *)(uintptr_t)(i+1));}
    unsigned count=0;
    while(count<4096&&(held[count]=malloc(65536))){
        ((unsigned char *)held[count])[0]=(unsigned char)count;++count;}
    CHECK(count>0&&count<4096&&errno==ENOMEM);
    CHECK(!realloc(held[0],32*1024*1024)&&errno==ENOMEM);
    for(unsigned i=0;i<count;++i){CHECK(((unsigned char *)held[i])[0]==(unsigned char)i);free(held[i]);}
    p=malloc(96);CHECK(p);free(p);
    say("Packed allocator collision/wrap/packing/zero/alignment/8workers/OOM PASS\n");return 91;
}
