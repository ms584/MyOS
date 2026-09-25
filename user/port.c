#include "app-lib.h"
/* Separate freestanding C program installed from a host-created data disk. */
__attribute__((noreturn)) void _start(void)
{
    static const char message[]="Ported C app: clock and console work\n";
    uint64_t before=myos_ticks();
    int64_t written=myos_write(message,sizeof(message)-1);
    uint64_t after=myos_ticks();
    myos_exit(written==sizeof(message)-1 && after>=before && before?7:1);
}
