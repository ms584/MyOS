/* Test-only adapter for compiler-rt emutls.c; not a compiler-rt runtime port. */
#ifndef MYOS_EMUTLS_TEST_INT_LIB_H
#define MYOS_EMUTLS_TEST_INT_LIB_H
#include <stdbool.h>
#define COMPILE_TIME_ASSERT(expression) _Static_assert(expression, #expression)
#endif
