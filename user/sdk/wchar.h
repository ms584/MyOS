#ifndef MYOS_SDK_WCHAR_H
#define MYOS_SDK_WCHAR_H
#include <stddef.h>
#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Stateless UTF-8 profile; no locale switching or stateful encodings. */
int wctomb(char *,wchar_t);
#ifdef __cplusplus
}
#endif
#endif
