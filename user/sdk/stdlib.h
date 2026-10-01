#ifndef MYOS_SDK_STDLIB_H
#define MYOS_SDK_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *malloc(size_t);
void free(void *);
#ifdef __cplusplus
}
#endif
#endif
