#include <stddef.h>
#include <stdint.h>

#define SYS_VM_MAP 14u
#define SYS_VM_UNMAP 15u
#define PROT_RW 3u
#define PAGE_SIZE 4096u
#define HEADER_MAGIC 0x4d594f53414c4c4full

struct allocation { uint64_t magic,base,length,requested; };

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
void *malloc(size_t size){return allocate(16,size,0);}
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
    if(count&&size>SIZE_MAX/count)return 0;
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
void *aligned_alloc(size_t alignment,size_t size){return allocate(alignment,size,1);}
int posix_memalign(void **result,size_t alignment,size_t size)
{
    if(!result||!power_of_two(alignment)||alignment%sizeof(void *))return 22;
    void *address=allocate(alignment,size,0);if(!address)return 12;
    *result=address;return 0;
}
