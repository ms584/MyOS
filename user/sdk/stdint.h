/* MyOS uses the selected compiler's target integer ABI, not host libc types.
 * GCC's intrinsic header supplies exact/least/fast types, limits and constants
 * from target predefined macros in both hosted and freestanding C/C++ modes. */
#ifndef MYOS_SDK_STDINT_H
#define MYOS_SDK_STDINT_H
#include <stdint-gcc.h>
#endif
