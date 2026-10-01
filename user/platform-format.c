#include "stdio_impl.h"
#include <errno.h>
#include <wchar.h>
int __lockfile(FILE *stream)
{if(pthread_mutex_lock(&stream->mutex))__builtin_trap();return 1;}
void __unlockfile(FILE *stream)
{if(pthread_mutex_unlock(&stream->mutex))__builtin_trap();}
int wctomb(char *buffer,wchar_t character)
{
    if(!buffer)return 0;
    unsigned value=(unsigned)character;
    if(value>0x10ffff||(value>=0xd800&&value<=0xdfff)){errno=EILSEQ;return -1;}
    if(value<0x80){buffer[0]=(char)value;return 1;}
    if(value<0x800){buffer[0]=(char)(0xc0|(value>>6));buffer[1]=(char)(0x80|(value&63));return 2;}
    if(value<0x10000){buffer[0]=(char)(0xe0|(value>>12));buffer[1]=(char)(0x80|((value>>6)&63));buffer[2]=(char)(0x80|(value&63));return 3;}
    buffer[0]=(char)(0xf0|(value>>18));buffer[1]=(char)(0x80|((value>>12)&63));
    buffer[2]=(char)(0x80|((value>>6)&63));buffer[3]=(char)(0x80|(value&63));return 4;
}
char *strerror(int error)
{
    const char *text;
    switch(error){
    case 0:text="No error information";break;
    case 2:text="No such file or directory";break;
    case 4:text="Interrupted system call";break;
    case 9:text="Bad file descriptor";break;
    case 11:text="Resource temporarily unavailable";break;
    case 12:text="Out of memory";break;
    case 13:text="Permission denied";break;
    case 14:text="Bad address";break;
    case 16:text="Resource busy";break;
    case 17:text="File exists";break;
    case 22:text="Invalid argument";break;
    case 28:text="No space left on device";break;
    case 33:text="Domain error";break;
    case 34:text="Result not representable";break;
    case 75:text="Value too large for data type";break;
    case 84:text="Illegal byte sequence";break;
    case 110:text="Operation timed out";break;
    default:text="Unknown error";break;
    }
    return (char *)text;
}
