#ifndef MYOS_STARBOARD_SABI_H
#define MYOS_STARBOARD_SABI_H
/* Exact Evergreen SABI descriptor from the pinned x64 Cobalt binary. This
 * rejects other boundaries, but is not a claim that every API is implemented. */
#define MYOS_STARBOARD_SABI \
 "{\"alignment_char\":1,\"alignment_double\":8,\"alignment_float\":4,\"alignment_int\":4," \
 "\"alignment_llong\":8,\"alignment_long\":8,\"alignment_pointer\":8,\"alignment_short\":2," \
 "\"calling_convention\":\"sysv\",\"endianness\":\"little\",\"floating_point_abi\":\"\"," \
 "\"floating_point_fpu\":\"\",\"sb_api_version\":18,\"signedness_of_char\":\"signed\"," \
 "\"signedness_of_enum\":\"unsigned\",\"size_of_char\":1,\"size_of_double\":8," \
 "\"size_of_enum\":4,\"size_of_float\":4,\"size_of_int\":4,\"size_of_llong\":8," \
 "\"size_of_long\":8,\"size_of_pointer\":8,\"size_of_short\":2,\"target_arch\":\"x64\"," \
 "\"target_arch_sub\":\"\",\"word_size\":64}"
enum myos_sabi_enum { MYOS_SABI_ENUM_VALUE = 1 };
_Static_assert(sizeof(void *)==8&&sizeof(long)==8&&sizeof(long long)==8&&
               sizeof(int)==4&&sizeof(short)==2&&sizeof(float)==4&&sizeof(double)==8&&
               sizeof(enum myos_sabi_enum)==4&&(char)-1<0&&
               (enum myos_sabi_enum)-1>0,"pinned SABI scalar sizes/signedness");
_Static_assert(_Alignof(char)==1&&_Alignof(short)==2&&_Alignof(int)==4&&
               _Alignof(float)==4&&_Alignof(double)==8&&_Alignof(long)==8&&
               _Alignof(long long)==8&&_Alignof(void *)==8,"pinned SABI alignment");
#if !defined(__x86_64__) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported Starboard SABI target
#endif
#endif
