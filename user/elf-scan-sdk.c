#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <fenv.h>
#include <wchar.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "scan-vectors.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"scan SDK failed: %s line %d\n",#x,__LINE__);__builtin_trap();}}while(0)
#ifdef __cplusplus
extern "C" {
#endif
void *__real_malloc(size_t);
void *__real_realloc(void *,size_t);
double _mesa_strtod(const char *,char **);
float _mesa_strtof(const char *,char **);
void *mmap(void *,size_t,int,int,int,int64_t);
int mprotect(void *,size_t,int);
int munmap(void *,size_t);
static int fail_malloc,fail_realloc;
void *__wrap_malloc(size_t size){if(fail_malloc){errno=ENOMEM;return 0;}return __real_malloc(size);}
void *__wrap_realloc(void *p,size_t size){if(fail_realloc){errno=ENOMEM;return 0;}return __real_realloc(p,size);}
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" int _start(void);
#endif
static void encode(char *text,uint64_t number,unsigned base)
{
    char reverse[64];size_t count=0;
    do{reverse[count++]="0123456789abcdefghijklmnopqrstuvwxyz"[number%base];number/=base;}while(number);
    for(size_t i=0;i<count;++i)text[i]=reverse[count-i-1];
    text[count]=0;
}
static void numbers(unsigned first,unsigned last)
{
    uint64_t state=UINT64_C(0x76543210fedcba98);
    for(unsigned base=first;base<=last;++base)for(unsigned i=0;i<96;++i){
        state=state*UINT64_C(6364136223846793005)+1;
        char text[70],*end=0;encode(text,state,base);errno=EIO;
        CHECK(strtoull(text,&end,(int)base)==state&&!*end&&errno==EIO);
        CHECK(strtoul(text,&end,(int)base)==state&&!*end);
        uint64_t signed_value=state>>1;encode(text,signed_value,base);
        CHECK(strtoll(text,&end,(int)base)==(int64_t)signed_value&&!*end);
        CHECK(strtol(text,&end,(int)base)==(long)signed_value&&!*end);
        char negative[72];negative[0]='-';strcpy(negative+1,text);
        CHECK(strtoll(negative,&end,(int)base)==-(int64_t)signed_value&&!*end);
    }
}
static void unicode(void)
{
    const char *texts[]={"A","\xc2\xa2","\xe0\xb8\x81","\xf0\x9f\x98\x80"};
    unsigned points[]={65,0xa2,0xe01,0x1f600};
    for(unsigned i=0;i<4;++i){
        mbstate_t state={0,0};wchar_t wc=0;size_t n=strlen(texts[i]);
        for(size_t j=0;j<n;++j){size_t got=mbrtowc(&wc,texts[i]+j,1,&state);
            CHECK(got==(j+1==n?1:(size_t)-2));}
        CHECK((unsigned)wc==points[i]&&mbsinit(&state));
        state=(mbstate_t){0,0};CHECK(mbrtowc(&wc,texts[i],n,&state)==n&&(unsigned)wc==points[i]);
    }
    const char *bad[]={"\xc0\x80","\xed\xa0\x80","\xf4\x90\x80\x80","\x80","\xe0\x80\x80"};
    for(unsigned i=0;i<5;++i){mbstate_t state={0,0};wchar_t wc=99;errno=EIO;
        CHECK(mbrtowc(&wc,bad[i],strlen(bad[i]),&state)==(size_t)-1&&errno==EILSEQ&&mbsinit(&state));}
}
static void *worker(void *argument)
{
    unsigned id=(unsigned)(uintptr_t)argument;int marker=40+(int)id;
    for(unsigned cycle=0;cycle<16;++cycle){
        char text[64],word[16];int value=0;double d=0;wchar_t wide[8];
        errno=marker;
        CHECK(sscanf("123 0x1.8p+2 word", "%d %la %15s",&value,&d,word)==3);
        CHECK(value==123&&d==6&&!strcmp(word,"word")&&errno==marker);
        CHECK(sscanf("\xe0\xb8\x81\xf0\x9f\x98\x80", "%7ls",wide)==1&&wide[0]==0xe01&&wide[1]==0x1f600&&!wide[2]);
        encode(text,(uint64_t)cycle*100+id,36);char *end=0;
        CHECK(strtoull(text,&end,36)==(uint64_t)cycle*100+id&&!*end);
        unicode();
    }
    return 0;
}
static int compare(const void *a,const void *b)
{int x=*(const int *)a,y=*(const int *)b;return (x>y)-(x<y);}
static void page_edges(void)
{
    char *mapping=(char *)mmap(0,8192,3,0x22,-1,0);CHECK(mapping!=(char *)(uintptr_t)-1);
    CHECK(!mprotect(mapping+4096,4096,0));
    const char *inputs[]={""," ","0","0x","0x1p","1e","1e+","nan(","nan(x)","infi","123.5","abc"};
    char long_input[602];unsigned lengths[]={31,255,256,257,300,511,600};
    for(unsigned i=0;i<19;++i){
        const char *input;
        if(i<12)input=inputs[i];
        else {memset(long_input,'1',lengths[i-12]);long_input[lengths[i-12]]=0;input=long_input;}
        size_t length=strlen(input);char *protected_text=mapping+4095-length;
        CHECK(!mprotect(mapping,4096,3));memcpy(protected_text,input,length+1);
        CHECK(!mprotect(mapping,4096,1));
        char *end1,*end2;errno=0;unsigned long long a=strtoull(input,&end1,0);int error1=errno;
        errno=0;unsigned long long b=strtoull(protected_text,&end2,0);
        CHECK(a==b&&end1-input==end2-protected_text&&error1==errno);
        union{double value;uint64_t bits;}x,y;
        errno=0;x.value=strtod(input,&end1);error1=errno;
        errno=0;y.value=strtod(protected_text,&end2);
        CHECK(x.bits==y.bits&&end1-input==end2-protected_text&&error1==errno);
        char ordinary[32],edge[32];memset(ordinary,0xa5,sizeof(ordinary));memset(edge,0xa5,sizeof(edge));
        CHECK(sscanf(input,"%31s",ordinary)==sscanf(protected_text,"%31s",edge));
        CHECK(!memcmp(ordinary,edge,sizeof(edge)));
    }
    CHECK(!munmap(mapping,8192));
}
int _start(void)
{
    CHECK(!fesetround(FE_TONEAREST));numbers(2,36);page_edges();
    char *end;const char *text=" \t-0x2aZ";errno=EIO;
    CHECK(strtol(text,&end,0)==-42&&*end=='Z'&&errno==EIO);
    text="077tail";CHECK(strtol(text,&end,0)==63&&end==text+3);
    text="0x";CHECK(!strtol(text,&end,0)&&end==text+1);
    text="  +!";errno=0;CHECK(!strtol(text,&end,10)&&end==text&&errno==EINVAL);
    text="123";errno=0;CHECK(!strtol(text,&end,1)&&end==text&&errno==EINVAL);
    errno=0;CHECK(strtoll("9223372036854775808",&end,10)==LLONG_MAX&&errno==ERANGE&&!*end);
    errno=0;CHECK(strtoll("-9223372036854775809",&end,10)==LLONG_MIN&&errno==ERANGE);
    errno=EIO;CHECK(strtoll("-9223372036854775808",&end,10)==LLONG_MIN&&errno==EIO);
    errno=0;CHECK(strtoull("18446744073709551616!",&end,10)==ULLONG_MAX&&errno==ERANGE&&*end=='!');
    CHECK(strtoull("-1",&end,10)==ULLONG_MAX&&!*end);
    CHECK(atoi(" -123!")==-123&&atol("+123!")==123&&atoll("123!")==123&&atof("1.25!")==1.25);
    CHECK(abs(-123)==123&&labs(-1234)==1234&&llabs(-12345)==12345);
    int sorted[257];for(unsigned i=0;i<257;++i)sorted[i]=(int)i*2;
    for(int key=-1;key<=515;++key){int *found=(int *)bsearch(&key,sorted,257,sizeof(int),compare);
        CHECK(found==(key>=0&&key<=512&&!(key&1)?sorted+key/2:0));}
    int key=0;CHECK(!bsearch(&key,sorted,0,sizeof(int),compare));
    for(size_t i=0;i<sizeof(scan_vectors)/sizeof(scan_vectors[0]);++i){
        union{double d;uint64_t bits;}d;union{float f;uint32_t bits;}f;
        d.d=strtod(scan_vectors[i].text,&end);CHECK(!*end&&d.bits==scan_vectors[i].double_bits);
        f.f=strtof(scan_vectors[i].text,&end);CHECK(!*end&&f.bits==scan_vectors[i].float_bits);
        d.d=_mesa_strtod(scan_vectors[i].text,&end);CHECK(!*end&&d.bits==scan_vectors[i].double_bits);
        f.f=_mesa_strtof(scan_vectors[i].text,&end);CHECK(!*end&&f.bits==scan_vectors[i].float_bits);
    }
    CHECK(strtold("0x1.0000000000000002p0",&end)==1.0L+0x1p-63L&&!*end);
    text="1e+!";CHECK(strtod(text,&end)==1&&end==text+1);
    text="word";CHECK(strtod(text,&end)==0&&end==text);
    CHECK(isinf(strtod("-INFINITY!",&end))&&*end=='!');
    CHECK(isnan(strtod("nan(payload)!",&end))&&*end=='!');
    errno=0;CHECK(isinf(strtod("1e9999",&end))&&errno==ERANGE);
    errno=0;CHECK(strtod("1e-9999",&end)==0&&errno==ERANGE);
    int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    const char *half="1.00000000000000011102230246251565404236316680908203125";
    for(unsigned i=0;i<4;++i){union{double d;uint64_t bits;}d;CHECK(!fesetround(modes[i]));
        d.d=strtod(half,&end);CHECK(d.bits==UINT64_C(0x3ff0000000000000)+(i==2));}
    CHECK(!fesetround(FE_TONEAREST));
    struct{uint64_t before;char text[12];uint64_t after;}guard;
    memset(&guard,0xa5,sizeof(guard));int number=0,count=-1;char letters[4]={0};
    CHECK(sscanf("42 abc,rest","%d %3[a-z]%n",&number,letters,&count)==2);
    CHECK(number==42&&!strcmp(letters,"abc")&&count==6);
    CHECK(sscanf("abcdef","%5s",guard.text)==1&&!strcmp(guard.text,"abcde"));
    CHECK(guard.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&guard.after==guard.before&&guard.text[6]==(char)0xa5);
    CHECK(sscanf("12 34","%*d %d",&number)==1&&number==34);
    short small;long large;long long largest;size_t size;double d;float f;long double ld;
    unsigned hex=0;unsigned char byte=0;
    CHECK(sscanf("0x89abcdef, ","0x%08x, ",&hex)==1&&hex==UINT32_C(0x89abcdef));
    CHECK(sscanf("fe","%2hhx",&byte)==1&&byte==254);
    CHECK(sscanf("-12 345 678 9 1.5 2.5 3.5","%hd %ld %lld %zu %f %lf %Lf",&small,&large,&largest,&size,&f,&d,&ld)==7);
    CHECK(small==-12&&large==345&&largest==678&&size==9&&f==1.5f&&d==2.5&&ld==3.5L);
    number=77;CHECK(sscanf("","%d",&number)==EOF&&number==77);
    CHECK(sscanf("word","%d",&number)==0&&number==77);
    char longword[301];memset(longword,'q',300);longword[300]=0;
    char *allocated=0;CHECK(sscanf(longword,"%ms",&allocated)==1&&allocated&&strlen(allocated)==300);free(allocated);
    allocated=(char *)1;fail_malloc=1;errno=0;
    CHECK(sscanf("abc","%ms",&allocated)==EOF&&allocated==(char *)1&&errno==ENOMEM);fail_malloc=0;
    fail_realloc=1;errno=0;
    CHECK(sscanf(longword,"%ms",&allocated)==EOF&&allocated==(char *)1&&errno==ENOMEM);fail_realloc=0;
    CHECK(sscanf("abc","%ms",&allocated)==1&&!strcmp(allocated,"abc"));free(allocated);
    unicode();pthread_t threads[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(threads+i,0,worker,(void *)(uintptr_t)i));
    for(unsigned i=0;i<8;++i)CHECK(!pthread_join(threads[i],0));
    puts("native sealed conversion/MPFR/scanf/UTF8/8workers/OOM PASS");return 88;
}
