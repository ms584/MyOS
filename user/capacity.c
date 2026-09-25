#include "app-lib.h"
/* Volatile access proves a mapped page beyond the first code page is readable. */
__attribute__((used,section(".rodata.capacity")))
static const unsigned char capacity_data[8192] = {[8191]='M'};
__attribute__((noreturn)) void _start(void)
{
    static const char message[]="Large C app crossed a code page\n";
    volatile const unsigned char *data=capacity_data;
    int ok=data[8191]=='M';
    int64_t written=myos_write(message,sizeof(message)-1);
    myos_exit(ok&&written==sizeof(message)-1?26:1);
}
