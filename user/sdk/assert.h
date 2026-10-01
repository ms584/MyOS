#ifndef MYOS_SDK_ASSERT_H
#define MYOS_SDK_ASSERT_H
/* Owned process fault, not a claim of SIGABRT/signal-handler support. */
#ifdef NDEBUG
#define assert(value) ((void)0)
#else
#define assert(value) ((value)?(void)0:__builtin_trap())
#endif
#endif
