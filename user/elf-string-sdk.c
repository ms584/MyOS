#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"string SDK failed: %s\n",#x);__builtin_trap();}}while(0)
#ifdef __cplusplus
extern "C" int _start(void);
#endif
static void *tokens(void *argument)
{
    (void)argument;
    for(unsigned cycle=0;cycle<128;++cycle){
        char a[]="::one,two;;;three:",b[]="red|green",*sa=0,*sb=0;
        CHECK(!strcmp(strtok_r(a,":,;",&sa),"one"));
        CHECK(!strcmp(strtok_r(b,"|",&sb),"red"));
        CHECK(!strcmp(strtok_r(0,":,;",&sa),"two"));
        CHECK(!strcmp(strtok_r(0,"|",&sb),"green"));
        CHECK(!strcmp(strtok_r(0,":,;",&sa),"three"));
        CHECK(!strtok_r(0,":,;",&sa)&&!strtok_r(0,"|",&sb));
    }
    return 0;
}
int _start(void)
{
    unsigned char source[256],expected[256],actual[256];
    for(unsigned i=0;i<256;++i)source[i]=(unsigned char)i;
    unsigned moves=0;
    for(unsigned length=0;length<=128;++length){
        for(unsigned from=16;from<=32;++from)for(unsigned to=16;to<=32;++to){
            memcpy(expected,source,256);memcpy(actual,source,256);
            for(unsigned i=0;i<length;++i)expected[to+i]=source[from+i];
            CHECK(memmove(actual+to,actual+from,length)==actual+to);
            CHECK(!memcmp(expected,actual,256));++moves;
        }
    }
    CHECK(moves==37281);
    for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){
        source[0]=(unsigned char)a;source[1]=(unsigned char)b;
        int result=memcmp(source,source+1,1);
        CHECK((result<0)==(a<b)&&(result>0)==(a>b));
    }
    source[0]=0xff;source[1]=0x7f;
    CHECK(memcmp(source,source+1,0)==0&&memchr(source,0x1ff,2)==source);
    const char text[]="a\xff" "b\xff";
    CHECK(strchr(text,0xff)==text+1&&strchr(text,0x1ff)==text+1);
    CHECK(strchr(text,0)==text+4&&!strchr(text,'X'));
    CHECK(strstr(text,"\xff" "b")==text+1&&strstr(text,"")==text);
    CHECK(!strstr(text,"\xff" "bb")&&!strstr("","x"));
    const char repeated[]="ababababx";
    CHECK(strstr(repeated,"ababx")==repeated+4);
    char original[]="copied\xff",*copy=strdup(original);CHECK(copy&&copy!=original);
    original[0]='X';CHECK(!strcmp(copy,"copied\xff"));free(copy);
    copy=strdup("");CHECK(copy&&!*copy);free(copy);
    char output[130];
    for(unsigned length=0;length<=128;++length){
        memset(output,'X',sizeof(output));
        CHECK(strncpy(output+1,"abc",length)==output+1);
        CHECK(output[0]=='X'&&output[length+1]=='X');
        for(unsigned i=0;i<length;++i)CHECK(output[i+1]==(i<3?"abc"[i]:0));
    }
    for(unsigned i=0;i<128;++i)source[i]='Q';
    source[128]=0;
    memset(output,'X',sizeof(output));strncpy(output+1,(char *)source,128);
    CHECK(output[0]=='X'&&output[129]=='X'&&output[128]=='Q');
    CHECK(strnlen((char *)source,127)==127&&strnlen((char *)source,129)==128);
    errno=EIO;
    for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){
        char first[2]={(char)a,0},second[2]={(char)b,0};
        int result=strncmp(first,second,1);
        CHECK((result<0)==(a<b)&&(result>0)==(a>b));
        CHECK(!strncmp(first,second,0));
        int aa=(a>='A'&&a<='Z')?(int)a+32:(int)a;
        int bb=(b>='A'&&b<='Z')?(int)b+32:(int)b;
        result=strcasecmp(first,second);
        CHECK((result<0)==(aa<bb)&&(result>0)==(aa>bb));
    }
    for(int c=-1;c<256;++c){
        int digit=c>='0'&&c<='9',lower=c>='a'&&c<='z',upper=c>='A'&&c<='Z';
        int alpha=lower||upper,graph=c>=33&&c<=126;
        CHECK(!!isdigit(c)==digit&&!!isalpha(c)==alpha&&!!isalnum(c)==(alpha||digit));
        CHECK(!!islower(c)==lower&&!!isupper(c)==upper);
        CHECK(!!isspace(c)==(c==' '||(c>=9&&c<=13)));
        CHECK(!!isblank(c)==(c==' '||c==9)&&!!iscntrl(c)==((c>=0&&c<32)||c==127));
        CHECK(!!isprint(c)==(c>=32&&c<=126)&&!!isgraph(c)==graph);
        CHECK(!!ispunct(c)==(graph&&!alpha&&!digit));
        CHECK(!!isxdigit(c)==(digit||(c>='a'&&c<='f')||(c>='A'&&c<='F')));
        CHECK(tolower(c)==(upper?c+32:c)&&toupper(c)==(lower?c-32:c));
    }
    CHECK(errno==EIO);
    CHECK(!strncasecmp("ABx","aby",2)&&strncasecmp("ABx","aby",3)<0);
    CHECK(strrchr(text,0x1ff)==text+3&&strrchr(text,0)==text+4&&!strrchr(text,'X'));
    CHECK(strchrnul(text,0x1ff)==text+1&&strchrnul(text,'X')==text+4&&strchrnul(text,0)==text+4);
    CHECK(strspn("abbaX","ab")==4&&strspn("abc","")==0);
    CHECK(strcspn("abcX","X")==3&&strcspn("abc","")==3);
    CHECK(strpbrk(text,"b")==text+2&&!strpbrk(text,""));
    for(unsigned bound=0;bound<10;++bound){
        memset(output,'X',sizeof(output));
        CHECK(strcpy(output+1,"ab")==output+1);
        CHECK(strncat(output+1,"cdef",bound)==output+1);
        unsigned length=2+(bound<4?bound:4);
        CHECK(output[0]=='X'&&output[length+2]=='X'&&!output[length+1]);
        CHECK(!strncmp(output+1,"abcdef",length));
    }
    memset(output,'X',sizeof(output));strcpy(output+1,"");
    CHECK(strcat(output+1,"xyz")==output+1&&!strcmp(output+1,"xyz"));
    CHECK(output[0]=='X'&&output[5]=='X');
    char bounded[3]={'a','b','c'};
    for(unsigned bound=0;bound<=3;++bound){
        copy=strndup(bounded,bound);CHECK(copy&&!strncmp(copy,bounded,bound)&&!copy[bound]);free(copy);
    }
    copy=strndup("a",SIZE_MAX);CHECK(copy&&!strcmp(copy,"a"));free(copy);
    char empty[]="",single[]="a,b",all[]="::::",no_delimiter[]="a,b";
    CHECK(!strtok(empty,",")&&!strtok(all,":"));
    CHECK(!strcmp(strtok(single,","),"a")&&!strcmp(strtok(0,""),"b")&&!strtok(0,","));
    CHECK(!strcmp(strtok(no_delimiter,""),"a,b")&&!strtok(0,""));
    pthread_t workers[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&workers[i],0,tokens,0));
    for(unsigned i=0;i<8;++i)CHECK(!pthread_join(workers[i],0));
    union{double d;uint64_t u;} pi={M_PI},log2e={M_LOG2E};
    CHECK(pi.u==UINT64_C(0x400921fb54442d18)&&log2e.u==UINT64_C(0x3ff71547652b82fe));
    for(size_t alignment=8;alignment<=65536;alignment*=2){
        void *pointer=(void *)1;errno=EIO;
        CHECK(!posix_memalign(&pointer,alignment,137)&&pointer);
        CHECK(!((uintptr_t)pointer&(alignment-1))&&errno==EIO);
        memset(pointer,0xa7,137);free(pointer);
        pointer=aligned_alloc(alignment,alignment*2);CHECK(pointer);
        CHECK(!((uintptr_t)pointer&(alignment-1)));free(pointer);
    }
    void *pointer=(void *)1;errno=EIO;
    CHECK(posix_memalign(&pointer,3,16)==EINVAL&&pointer==(void *)1&&errno==EIO);
    CHECK(posix_memalign(&pointer,16,SIZE_MAX)==ENOMEM&&pointer==(void *)1&&errno==EIO);
    CHECK(!aligned_alloc(16,17)&&errno==EINVAL);
    CHECK(!calloc(SIZE_MAX,2)&&errno==ENOMEM);
    unsigned char *zero=(unsigned char *)calloc(17,19);CHECK(zero);
    for(unsigned i=0;i<323;++i)CHECK(!zero[i]);
    memset(zero,0xb3,323);free(zero);
    zero=(unsigned char *)calloc(17,19);CHECK(zero);
    for(unsigned i=0;i<323;++i)CHECK(!zero[i]);
    free(zero);
    puts("native SDK strings/37281 overlaps/unsigned compare/allocator ABI PASS");
    puts("native string extensions/C-locale/8-worker tokens/math constants PASS");
    return 82;
}
