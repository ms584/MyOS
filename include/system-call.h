#ifndef MYOS_SYSTEM_CALL_H
#define MYOS_SYSTEM_CALL_H
#include <stdint.h>
#define MYOS_SYSTEM_CALL 51u
#define SYSTEM_LOG_LIMIT 65536u
enum {SYSTEM_LOG=0,SYSTEM_TOTAL_BYTES,SYSTEM_OWNED_BYTES,SYSTEM_RANDOM,SYSTEM_DISCONNECTED,SYSTEM_PATH};
struct system_path_request {int32_t id,length;uint64_t buffer;};
_Static_assert(sizeof(struct system_path_request)==16,"system syscall request ABI");
#endif
