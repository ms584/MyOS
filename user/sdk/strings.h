#ifndef MYOS_SDK_STRINGS_H
#define MYOS_SDK_STRINGS_H
#include <stddef.h>
/* Guest C-locale ASCII folding, not host locale state. */
#ifdef __cplusplus
extern "C" {
#endif
int strcasecmp(const char *,const char *);
int strncasecmp(const char *,const char *,size_t);
#ifdef __cplusplus
}
#endif
#endif
