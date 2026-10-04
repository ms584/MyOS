#ifndef MYOS_STARBOARD_SYSTEM_CONFIGURATION_H
#define MYOS_STARBOARD_SYSTEM_CONFIGURATION_H
#define SB_API_VERSION 18
#define SB_C_FORCE_INLINE inline
#define SB_NORETURN _Noreturn
#define SB_PRINTF_FORMAT(format_index,args_index) __attribute__((format(printf,format_index,args_index)))
#endif
