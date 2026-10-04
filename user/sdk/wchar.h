#ifndef MYOS_SDK_WCHAR_H
#define MYOS_SDK_WCHAR_H
#include <stddef.h>
#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif
typedef struct {unsigned state,reserved;} mbstate_t;
#ifdef __cplusplus
extern "C" {
#endif
/* UTF-8-only guest profile; mbrtowc uses explicit partial-sequence state.
 * No locale switching or other/stateful encodings. */
int wctomb(char *,wchar_t);
size_t mbrtowc(wchar_t *,const char *,size_t,mbstate_t *);
int mbsinit(const mbstate_t *);
size_t wcrtomb(char *,wchar_t,mbstate_t *);
size_t mbsrtowcs(wchar_t *,const char **,size_t,mbstate_t *);
size_t wcsrtombs(char *,const wchar_t **,size_t,mbstate_t *);
#ifdef __cplusplus
}
#endif
#endif
