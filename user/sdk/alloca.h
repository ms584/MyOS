#ifndef MYOS_SDK_ALLOCA_H
#define MYOS_SDK_ALLOCA_H
/* Real compiler stack allocation, released by return or restored longjmp.
 * Callers must bound allocation; existing native stack guards still apply. */
#include <stddef.h>
#define alloca __builtin_alloca
#endif
