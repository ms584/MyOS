#include <stddef.h>
#include <stdint.h>

#define SYS_VM_MAP 14u
#define SYS_VM_UNMAP 15u
#define SYS_VM_PROTECT 16u
#define SYS_VM_QUERY 22u
#define SYS_VM_ADVISE 23u
#define SYS_ERRNO_LOCATION 24u
#define SYS_VM_MAP_FIXED 25u
#define PROT_RW 3u
#define PAGE_SIZE 4096u
#define HEADER_MAGIC 0x4d594f53414c4c4full

struct allocation { uint64_t magic,base,length,requested; };
int *__errno_location(void);

static int64_t call3(uint64_t number,uint64_t first,uint64_t second,uint64_t third)
{
    register uint64_t a __asm__("rax")=number;
    register uint64_t d __asm__("rdi")=first;
    register uint64_t s __asm__("rsi")=second;
    register uint64_t x __asm__("rdx")=third;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s),"d"(x):"memory","cc");
    return (int64_t)a;
}
static int power_of_two(size_t value){return value&&!(value&(value-1));}
static void *allocate(size_t alignment,size_t size,int exact)
{
    if(!size||!power_of_two(alignment)||alignment<sizeof(void *))return 0;
    if(exact&&size%alignment)return 0;
    if(size>SIZE_MAX-sizeof(struct allocation)-alignment)return 0;
    size_t need=size+sizeof(struct allocation)+alignment-1;
    if(need>SIZE_MAX-(PAGE_SIZE-1))return 0;
    size_t length=(need+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    int64_t mapped=call3(SYS_VM_MAP,0,length,PROT_RW);
    if(mapped<0)return 0;
    uintptr_t address=((uintptr_t)mapped+sizeof(struct allocation)+alignment-1)&~(uintptr_t)(alignment-1);
    struct allocation *header=(struct allocation *)address-1;
    *header=(struct allocation){HEADER_MAGIC,(uint64_t)mapped,length,size};
    return (void *)address;
}
void *malloc(size_t size){void *p=allocate(16,size,0);if(size&&!p)*__errno_location()=12;return p;}
void free(void *address)
{
    if(!address)return;
    struct allocation *header=(struct allocation *)address-1;
    if(header->magic!=HEADER_MAGIC)return;
    uint64_t base=header->base,length=header->length;header->magic=0;
    (void)call3(SYS_VM_UNMAP,base,length,0);
}
void *calloc(size_t count,size_t size)
{
    if(count&&size>SIZE_MAX/count){*__errno_location()=12;return 0;}
    return malloc(count*size);
}
size_t malloc_usable_size(void *address)
{
    if(!address)return 0;
    struct allocation *header=(struct allocation *)address-1;
    return header->magic==HEADER_MAGIC?(size_t)header->requested:0;
}
void *realloc(void *address,size_t size)
{
    if(!address)return malloc(size);
    if(!size){free(address);return 0;}
    size_t old=malloc_usable_size(address);if(!old)return 0;
    void *replacement=malloc(size);if(!replacement)return 0;
    size_t count=old<size?old:size;unsigned char *dst=replacement,*src=address;
    for(size_t i=0;i<count;++i)dst[i]=src[i];
    free(address);return replacement;
}
void *aligned_alloc(size_t alignment,size_t size)
{
    if(!power_of_two(alignment)||alignment<sizeof(void *)||size%alignment){
        *__errno_location()=22;return 0;
    }
    void *p=allocate(alignment,size,1);if(size&&!p)*__errno_location()=12;return p;
}
int posix_memalign(void **result,size_t alignment,size_t size)
{
    if(!result||!power_of_two(alignment)||alignment%sizeof(void *))return 22;
    void *address=allocate(alignment,size,0);if(!address)return 12;
    *result=address;return 0;
}


#define POSIX_PROT_READ 1
#define POSIX_PROT_WRITE 2
#define POSIX_PROT_EXEC 4
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000
#define MAP_STACK 0x20000
#define MAP_FIXED_NOREPLACE 0x100000

int *__errno_location(void)
{
    return (int *)(uintptr_t)call3(SYS_ERRNO_LOCATION,0,0,0);
}
static int fail(int64_t error)
{
    *__errno_location()=(int)-error;
    return -1;
}
static int vm_prot(int prot)
{
    if(prot&~7)return -1;
    if((prot&POSIX_PROT_WRITE)&&(prot&POSIX_PROT_EXEC))return -1;
    if(prot&POSIX_PROT_WRITE)prot|=POSIX_PROT_READ;
    if(prot&POSIX_PROT_EXEC)prot|=POSIX_PROT_READ;
    return prot;
}
static int rounded(size_t length,size_t *result)
{
    if(!length||length>SIZE_MAX-(PAGE_SIZE-1))return 0;
    *result=(length+PAGE_SIZE-1)&~(size_t)(PAGE_SIZE-1);
    return 1;
}
void *mmap(void *address,size_t length,int prot,int flags,int fd,int64_t offset)
{
    size_t bytes;int translated=vm_prot(prot);
    int allowed=MAP_SHARED|MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS|MAP_NORESERVE|MAP_STACK|MAP_FIXED_NOREPLACE;
    if(translated<0||!rounded(length,&bytes)||(flags&~allowed)||
       ((!!(flags&MAP_PRIVATE))==(!!(flags&MAP_SHARED)))||!(flags&MAP_ANONYMOUS)||
       fd!=-1||offset||((flags&(MAP_FIXED|MAP_FIXED_NOREPLACE))&&
       ((uintptr_t)address&(PAGE_SIZE-1)))){
        *__errno_location()=22;return (void *)(uintptr_t)-1;
    }
    uint64_t hint=(flags&(MAP_FIXED|MAP_FIXED_NOREPLACE))?(uint64_t)(uintptr_t)address:0;
    uint64_t syscall=(flags&MAP_FIXED)?SYS_VM_MAP_FIXED:SYS_VM_MAP;
    int64_t result=call3(syscall,hint,bytes,(unsigned)translated);
    if(result<0){fail(result);return (void *)(uintptr_t)-1;}
    return (void *)(uintptr_t)result;
}
int mprotect(void *address,size_t length,int prot)
{
    size_t bytes;int translated=vm_prot(prot);
    if(translated<0||((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_PROTECT,(uintptr_t)address,bytes,(unsigned)translated);
    return result<0?fail(result):0;
}
int munmap(void *address,size_t length)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_UNMAP,(uintptr_t)address,bytes,0);
    return result<0?fail(result):0;
}
int mincore(void *address,size_t length,unsigned char *vector)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_QUERY,(uintptr_t)address,bytes,(uintptr_t)vector);
    return result<0?fail(result):0;
}
int madvise(void *address,size_t length,int advice)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)){
        *__errno_location()=22;return -1;
    }
    int64_t result=call3(SYS_VM_ADVISE,(uintptr_t)address,bytes,(unsigned)advice);
    return result<0?fail(result):0;
}
int msync(void *address,size_t length,int flags)
{
    size_t bytes;
    if(((uintptr_t)address&(PAGE_SIZE-1))||!rounded(length,&bytes)||
       (flags&~7)||!((flags&1)^(flags&4))){
        *__errno_location()=22;return -1;
    }
    unsigned char byte;int64_t result=0;
    for(size_t offset=0;offset<bytes;offset+=PAGE_SIZE){
        result=call3(SYS_VM_QUERY,(uintptr_t)address+offset,PAGE_SIZE,(uintptr_t)&byte);
        if(result<0)break;
    }
    return result<0?fail(result):0;
}
