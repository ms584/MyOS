#include "app-lib.h"
__attribute__((noreturn)) void _start(void)
{
    static const char message[]="Hello from /bin/hello (C, Ring 3)\n";
    myos_exit(myos_write(message,sizeof(message)-1)==sizeof(message)-1?42:1);
}
