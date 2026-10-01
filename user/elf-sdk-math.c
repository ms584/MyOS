#define _GNU_SOURCE
#include <math.h>
#include <fenv.h>
#include <stdint.h>
#include "c11/threads.h"
#include "math-vectors.h"
static unsigned current;
static void message(const char *text,unsigned length)
{
    register uint64_t a __asm__("rax")=1,d __asm__("rdi")=(uintptr_t)text;
    register uint64_t s __asm__("rsi")=length;
    __asm__ volatile("int $0x80":"+a"(a):"D"(d),"S"(s):"rdx","memory","cc");
}
static void number(unsigned value)
{
    char text[16];unsigned length=0;
    do{text[length++]=(char)('0'+value%10);value/=10;}while(value);
    for(unsigned i=0;i<length/2;++i){char ch=text[i];text[i]=text[length-1-i];text[length-1-i]=ch;}
    message(text,length);
}
static void fail(unsigned line)
{
    const char text[]="native math failure line ";message(text,sizeof(text)-1);number(line);
    const char index[]=" case ";message(index,sizeof(index)-1);number(current);message("\n",1);
    __builtin_trap();
}
#define CHECK(x) do { if(!(x))fail(__LINE__); } while(0)
static double value(uint64_t bits){union{uint64_t bits;double value;} u={bits};return u.value;}
static uint64_t bits(double value){union{double value;uint64_t bits;} u={value};return u.bits;}
static uint64_t ordered(uint64_t bits,unsigned single)
{
    uint64_t sign=single?UINT64_C(0x80000000):UINT64_C(0x8000000000000000);
    uint64_t mask=single?UINT64_C(0xffffffff):UINT64_MAX;
    return bits&sign?(~bits)&mask:bits|sign;
}
static int fp_worker(void *raw)
{
    unsigned who=(unsigned)(uintptr_t)raw;
    const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    const double positive[]={2,1,2,1},negative[]={-2,-2,-1,-1};
    CHECK(!fesetround(modes[who]));CHECK(!feclearexcept(FE_ALL_EXCEPT));
    int flag=(who&1)?FE_INVALID:FE_DIVBYZERO;CHECK(!feraiseexcept(flag));
    for(unsigned i=0;i<12;++i){
        struct timespec delay={0,2000000};CHECK(!thrd_sleep(&delay,0));
        CHECK(fegetround()==modes[who]);
        volatile double x=1.5,y=-1.5;
        CHECK(rint(x)==positive[who]&&rint(y)==negative[who]);
        CHECK(fetestexcept(FE_INVALID|FE_DIVBYZERO)==flag);
    }
    return (int)who+100;
}
int _start(void)
{
    CHECK(!fesetround(FE_TONEAREST));
    for(current=0;current<sizeof(cases)/sizeof(cases[0]);++current){
        const struct math_case *c=&cases[current];
        double expected=value(c->expected),actual=evaluate(c->operation,value(c->x),value(c->y),value(c->z));
        if(isnan(expected)){CHECK(isnan(actual));continue;}
        CHECK(!isnan(actual));
        if(expected==0||isinf(expected)){CHECK(bits(expected)==bits(actual));continue;}
        uint64_t want=bits(expected),got=bits(actual);
        if(c->is_float){union{float value;uint32_t bits;} a={(float)actual},e={(float)expected};
            want=e.bits;got=a.bits;}
        want=ordered(want,c->is_float);got=ordered(got,c->is_float);
        CHECK((want>got?want-got:got-want)<=c->ulps);
    }
    int exponent;CHECK(frexp(8,&exponent)==0.5&&exponent==4);
    CHECK(frexpf(0x1p-149f,&exponent)==0.5f&&exponent==-148);
    CHECK(ldexp(0.5,4)==8&&ldexpf(0.5f,4)==8);
    CHECK(scalbn(1,-1074)==0x1p-1074&&scalbnf(1,-149)==0x1p-149f);
    CHECK(fmin(NAN,2)==2&&fmax(2,NAN)==2);
    CHECK(fminf(NAN,2)==2&&fmaxf(2,NAN)==2);
    float sine,cosine;sincosf(0,&sine,&cosine);CHECK(sine==0&&cosine==1);
    CHECK(!feclearexcept(FE_ALL_EXCEPT));
    volatile double minus=-1,zero=0;
    CHECK(isnan(sqrt(minus))&&(fetestexcept(FE_INVALID)&FE_INVALID));
    CHECK(!feclearexcept(FE_ALL_EXCEPT));
    CHECK(isinf(log2(zero))&&(fetestexcept(FE_DIVBYZERO)&FE_DIVBYZERO));
    CHECK(!feclearexcept(FE_ALL_EXCEPT));
    thrd_t threads[4];for(unsigned i=0;i<4;++i)CHECK(thrd_create(&threads[i],fp_worker,(void *)(uintptr_t)i)==thrd_success);
    for(unsigned i=0;i<4;++i){int result;CHECK(thrd_join(threads[i],&result)==thrd_success&&result==(int)i+100);}
    CHECK(fegetround()==FE_TONEAREST&&fetestexcept(FE_INVALID|FE_DIVBYZERO)==0);
    const char passed[]="native musl math MPFR vectors/rounding/preemption PASS\n";
    message(passed,sizeof(passed)-1);return 72;
}
