#include <stddef.h>
#include <stdint.h>
#include "app-lib.h"
struct block {size_t size;struct block *next;int free;};
static struct block *head;
void *malloc(size_t size)
{
    if(!size||size>MYOS_HEAP_BYTES-sizeof(struct block))return 0;
    size=(size+15)&~(size_t)15;
    if(!head){head=(void *)MYOS_HEAP_BASE;*head=(struct block){MYOS_HEAP_BYTES-sizeof(*head),0,1};}
    for(struct block *p=head;p;p=p->next)if(p->free&&p->size>=size){
        if(p->size>=size+sizeof(*p)+16){
            struct block *n=(void *)((char *)(p+1)+size);
            *n=(struct block){p->size-size-sizeof(*n),p->next,1};
            p->next=n;p->size=size;
        }
        p->free=0;return p+1;
    }
    return 0;
}
void free(void *address)
{
    if(!address)return;
    for(struct block *p=head;p;p=p->next)if((void *)(p+1)==address){
        p->free=1;
        while(p->next&&p->next->free){p->size+=sizeof(*p)+p->next->size;p->next=p->next->next;}
        for(struct block *q=head;q&&q->next!=p;q=q->next)
            if(q->next==p&&q->free){q->size+=sizeof(*q)+p->size;q->next=p->next;}
        return;
    }
}
void *memcpy(void *dst,const void *src,size_t n)
{unsigned char *d=dst;const unsigned char *s=src;for(size_t i=0;i<n;++i)d[i]=s[i];return dst;}
void *memset(void *dst,int value,size_t n)
{unsigned char *d=dst;for(size_t i=0;i<n;++i)d[i]=(unsigned char)value;return dst;}
int strncmp(const char *a,const char *b,size_t n)
{for(size_t i=0;i<n;++i){unsigned char x=a[i],y=b[i];if(x!=y)return (int)x-(int)y;if(!x)return 0;}return 0;}
