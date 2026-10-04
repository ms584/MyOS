#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "util/os_misc.h"
#include "util/u_dl.h"
#ifdef __cplusplus
extern "C" {
#endif
void *mmap(void *,size_t,int,int,int,int64_t);
int munmap(void *,size_t);
#ifdef __cplusplus
}
#endif
static void say(const char *text,size_t length)
{(void)write(1,text,length);}
#define SAY(s) say(s,sizeof(s)-1)
#define CHECK(x) do{if(!(x)){SAY("os-info failed: " #x "\n");__builtin_trap();}}while(0)
static long raw_memory(unsigned selector)
{
    register uint64_t a __asm__("rax")=50,d __asm__("rdi")=selector;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d):"memory","cc");
    return (long)a;
}
static void report(long total,long free)
{
    char text[80];size_t pos=0;
    const char prefix[]="native managed pages=";
    for(size_t i=0;i<sizeof(prefix)-1;++i)text[pos++]=prefix[i];
    long values[]={total,free};
    for(unsigned j=0;j<2;++j){
        char digits[24];unsigned count=0;unsigned long value=(unsigned long)values[j];
        do{digits[count++]=(char)('0'+value%10);value/=10;}while(value);
        while(count)text[pos++]=digits[--count];
        text[pos++]=j?'\n':' ';
    }
    say(text,pos);
}
static void check_info(void)
{
    uint64_t total=0,page=0,available=0;
    CHECK(os_get_total_physical_memory(&total)&&os_get_page_size(&page));
    CHECK(page==4096&&total==(uint64_t)sysconf(_SC_PHYS_PAGES)*page);
    CHECK(os_get_available_system_memory(&available));
    CHECK(available<=total&&available%page==0);
    CHECK(sysconf(_SC_PHYS_PAGES)==raw_memory(0));
}
static void *worker(void *argument)
{
    for(unsigned i=0;i<64;++i){
        check_info();void *p=malloc(1024+i*17);CHECK(p);free(p);
    }
    return argument;
}
static void workers(void)
{
    pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)(i+1)));
    for(unsigned i=0;i<8;++i){void *result=0;
        CHECK(!pthread_join(threads[i],&result)&&result==(void *)(uintptr_t)(i+1));}
}
#ifdef __cplusplus
extern "C"
#endif
int _start(void)
{
    errno=73;CHECK(sysconf(-1)==-1&&errno==EINVAL);
    CHECK(raw_memory(2)==-EINVAL&&raw_memory(0xffffffffu)==-EINVAL);
    errno=73;CHECK(sysconf(_SC_PAGESIZE)==4096&&errno==73);
    long total=sysconf(_SC_PHYS_PAGES);
    CHECK(total>0&&total<=65536&&errno==73);
    /* Empty anonymous page tables intentionally live until process reap.
     * Check exact data-page return, then warm stack/allocator mappings before
     * asserting repeated mapping/worker cycles consume no further pages. */
    void *warm=mmap(0,65*4096,3,34,-1,0);CHECK(warm!=(void *)(uintptr_t)-1);
    long mapped=sysconf(_SC_AVPHYS_PAGES);CHECK(!munmap(warm,65*4096));
    CHECK(sysconf(_SC_AVPHYS_PAGES)==mapped+65);workers();
    long before=sysconf(_SC_AVPHYS_PAGES);CHECK(before>0&&before<total);
    report(total,before);check_info();
    const unsigned counts[]={1,17,65};
    for(unsigned i=0;i<3;++i){unsigned pages=counts[i];
        long free_before=sysconf(_SC_AVPHYS_PAGES);
        size_t length=(size_t)pages*4096;
        volatile unsigned char *p=(volatile unsigned char *)mmap(0,length,3,34,-1,0);
        CHECK(p!=(void *)(uintptr_t)-1);
        p[0]=11;p[length-1]=22;
        long during=sysconf(_SC_AVPHYS_PAGES);
        CHECK(during<=free_before-(long)pages&&sysconf(_SC_PHYS_PAGES)==total);
        uint64_t available=0;CHECK(os_get_available_system_memory(&available));
        CHECK(available==(uint64_t)during*4096&&p[0]==11&&p[length-1]==22);
        CHECK(!munmap((void *)p,length));
        CHECK(sysconf(_SC_AVPHYS_PAGES)==free_before);
    }
    workers();
    CHECK(sysconf(_SC_PHYS_PAGES)==total&&sysconf(_SC_AVPHYS_PAGES)==before);
    /* Preserve upstream's truthful unavailable dynamic-loader contract. */
    CHECK(!util_dl_open("not-a-native-plugin.so"));
    CHECK(!util_dl_get_proc_address(0,"no_symbol"));util_dl_close(0);
    CHECK(util_dl_error()!=0);
    SAY("native Mesa real memory/page queries and unavailable dl PASS\n");
    return 96;
}
