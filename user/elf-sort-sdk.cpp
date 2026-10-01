#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "u_qsort.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"sort failed: %s\n",#x);__builtin_trap();}}while(0)
static int expected[1024],observed[1024];
static int compare(const void *a,const void *b)
{int x=*(const int *)a,y=*(const int *)b;return (x>y)-(x<y);}
struct context {int direction,nested;};
static int nested_compare(const void *a,const void *b,void *raw)
{
    context *c=(context *)raw;
    if(!c->nested){
        c->nested=1;int inner[5]={1,4,2,5,3};context other={-1,1};
        util_tls_qsort_r(inner,5,sizeof(int),nested_compare,&other);
        for(int i=0;i<5;++i)CHECK(inner[i]==5-i);
    }
    return c->direction*compare(a,b);
}
static void *worker(void *raw)
{
    unsigned id=(unsigned)(uintptr_t)raw;
    for(unsigned round=0;round<12;++round){
        int a[257];for(int i=0;i<257;++i)a[i]=256-i;
        context c={id&1?-1:1,0};util_tls_qsort_r(a,257,sizeof(int),nested_compare,&c);
        for(int i=0;i<257;++i)CHECK(a[i]==(id&1?256-i:i));
    }
    for(unsigned i=0;i<128;++i)observed[id*128+i]=rand();
    return raw;
}
static void finalize1(void){puts("sort finalize1");}
static void finalize2(void){puts("sort finalize2");}
struct record {int key;unsigned char payload[1533];};
static int record_compare(const void *a,const void *b)
{const record *x=(const record *)a,*y=(const record *)b;return (x->key>y->key)-(x->key<y->key);}
extern "C" int _start(void)
{
    CHECK(rand()==0); /* fresh image's default musl seed */
    const unsigned seeds[]={0u,1u,123u,UINT32_MAX};
    for(unsigned seed: seeds){
        srand(seed);uint64_t state=(unsigned)(seed-1);
        for(unsigned i=0;i<128;++i){state=UINT64_C(6364136223846793005)*state+1;CHECK(rand()==(int)(state>>33));}
    }
    qsort(0,0,sizeof(int),compare);
    int contextual[3]={2,0,1};context direct={1,0};
    qsort_r(contextual,3,sizeof(int),nested_compare,&direct);
    CHECK(contextual[0]==0&&contextual[1]==1&&contextual[2]==2);
    for(unsigned n=0;n<258;++n){
        int a[259];a[0]=a[n+1]=123456;
        for(unsigned i=0;i<n;++i)a[i+1]=(int)(n-1-i);
        qsort(a+1,n,sizeof(int),compare);
        CHECK(a[0]==123456&&a[n+1]==123456);
        for(unsigned i=0;i<n;++i)CHECK(a[i+1]==(int)i);
    }
    unsigned char *allocation=(unsigned char *)malloc(65*sizeof(record)+32);CHECK(allocation);
    memset(allocation,0xa5,65*sizeof(record)+32);record *r=(record *)(allocation+16);
    for(unsigned i=0;i<65;++i){r[i].key=64-i;memset(r[i].payload,(unsigned char)(64-i),sizeof(r[i].payload));}
    qsort(r,65,sizeof(record),record_compare);
    for(unsigned i=0;i<65;++i){CHECK(r[i].key==(int)i);for(unsigned j=0;j<sizeof(r[i].payload);++j)CHECK(r[i].payload[j]==i);}
    for(unsigned i=0;i<16;++i)CHECK(allocation[i]==0xa5&&allocation[16+65*sizeof(record)+i]==0xa5);
    free(allocation);
    srand(123);for(unsigned i=0;i<1024;++i)expected[i]=rand();
    srand(123);pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&threads[i],0,worker,(void *)(uintptr_t)i));
    for(unsigned i=0;i<8;++i){void *value;CHECK(!pthread_join(threads[i],&value)&&value==(void *)(uintptr_t)i);}
    qsort(expected,1024,sizeof(int),compare);qsort(observed,1024,sizeof(int),compare);
    CHECK(!memcmp(expected,observed,sizeof(expected)));
    CHECK(!atexit(finalize1)&&!atexit(finalize2));
    puts("native sort/random/Mesa TLS nested/concurrency PASS");return 86;
}
