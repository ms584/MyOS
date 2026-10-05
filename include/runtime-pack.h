#ifndef MYOS_RUNTIME_PACK_H
#define MYOS_RUNTIME_PACK_H
#include <stdbool.h>
#include <stddef.h>
#define RUNTIME_PACK_LIMIT 65536u
/* MRT1 header: magic, uint32 raw bytes, packed bytes, raw CRC32 (little-endian).
 * Failure may leave private destination partially filled. Never publish/map it
 * before success. No allocations, globals, host ABI or input-driven recursion. */
bool runtime_unpack(const void *,size_t,void *,size_t);
#endif
