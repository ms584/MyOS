#include "os.h"
void *memset(void *p,int c,size_t n){unsigned char *s=p;while(n--)*s++=(unsigned char)c;return p;}
void *memcpy(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}
_Noreturn void panic(const char *s){__asm__ volatile("cli");print("\nPANIC: ");print(s);print("\n");for(;;)__asm__ volatile("hlt");}
void check(bool ok,const char *s){if(!ok)panic(s);}
