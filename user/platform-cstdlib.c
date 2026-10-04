#include <stdlib.h>
#include <errno.h>
/* MyOS has no subprocess command interpreter in this native SDK profile. */
int system(const char *command)
{
    if(!command)return 0;
    errno=ENOSYS;
    return -1;
}
