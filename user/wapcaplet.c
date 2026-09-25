#include "app-lib.h"
#include "libwapcaplet/libwapcaplet.h"
static volatile uint64_t bss_probe[8];
__attribute__((noreturn)) void _start(void)
{
    lwc_string *a=0,*b=0,*c=0;
    bool equal=false,caseless=false;
    int zero=bss_probe[2]==0&&bss_probe[3]==0;
    int ok=zero&&lwc_intern_string("Browser",7,&a)==lwc_error_ok;
    ok=ok&&lwc_intern_string("Browser",7,&b)==lwc_error_ok;
    ok=ok&&lwc_string_isequal(a,b,&equal)==lwc_error_ok&&equal;
    ok=ok&&lwc_intern_string("browser",7,&c)==lwc_error_ok;
    ok=ok&&lwc_string_caseless_isequal(a,c,&caseless)==lwc_error_ok&&caseless;
    if(a)lwc_string_unref(a);
    if(b)lwc_string_unref(b);
    if(c)lwc_string_unref(c);
    lwc_iterate_strings(0,0);
    static const char message[]="NetSurf libwapcaplet Ring 3: string interning PASS\n";
    if(ok)myos_write(message,sizeof(message)-1);
    myos_exit(ok?26:1);
}
