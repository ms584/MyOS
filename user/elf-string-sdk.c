#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"string SDK failed: %s\n",#x);__builtin_trap();}}while(0)
#ifdef __cplusplus
extern "C" int _start(void);
#endif
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
    return 82;
}
