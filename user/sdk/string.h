#ifndef MYOS_SDK_STRING_H
#define MYOS_SDK_STRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *memcpy(void *,const void *,size_t);
size_t strlen(const char *);
#ifdef __cplusplus
}
#endif
#endif
