#include "stdio_impl.h"
/* Used only with vsscanf's real string reader. Conversion pseudo-streams
 * use their direct NUL-terminated buffer and never request a refill. */
int __myos_memory_toread(FILE *f)
{f->rpos=f->rend=f->buf;return 0;}
int __myos_memory_uflow(FILE *f)
{
    unsigned char byte;
    return f->read&&f->read(f,&byte,1)==1?byte:EOF;
}
