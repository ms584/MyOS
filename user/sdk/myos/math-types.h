/* MyOS math-only evaluation types. No musl/Linux OS types are exposed. */
#ifndef MYOS_MATH_TYPES_H
#define MYOS_MATH_TYPES_H
#if __FLT_EVAL_METHOD__ == 0
typedef float float_t;
typedef double double_t;
#elif __FLT_EVAL_METHOD__ == 1
typedef double float_t;
typedef double double_t;
#elif __FLT_EVAL_METHOD__ == 2
typedef long double float_t;
typedef long double double_t;
#else
#error Unsupported MyOS floating-point evaluation method
#endif
#endif
