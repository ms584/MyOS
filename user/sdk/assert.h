#ifndef MYOS_SDK_ASSERT_H
#define MYOS_SDK_ASSERT_H
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L
#define static_assert _Static_assert
#endif
/* Owned process fault, not a claim of SIGABRT/signal-handler support. */
#ifdef NDEBUG
#define assert(value) ((void)0)
#else
#define assert(value) ((value)?(void)0:__builtin_trap())
#endif
#endif
