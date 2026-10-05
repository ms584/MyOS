#ifndef MYOS_SYSTEM_CALL_H
#define MYOS_SYSTEM_CALL_H
#include <stdint.h>
#define MYOS_SYSTEM_CALL 51u
#define SYSTEM_LOG_LIMIT 65536u
enum {SYSTEM_LOG=0,SYSTEM_TOTAL_BYTES,SYSTEM_OWNED_BYTES,SYSTEM_RANDOM,SYSTEM_DISCONNECTED,SYSTEM_PATH,SYSTEM_INTERFACE,SYSTEM_ASSET_ENTRY};
struct system_asset_entry_request {uint64_t path,position,output;};
struct system_asset_entry {char name[240];uint32_t type,reserved;uint64_t size;};
_Static_assert(sizeof(struct system_asset_entry_request)==24&&sizeof(struct system_asset_entry)==256,"asset syscall ABI");
struct system_interface_snapshot {
    uint32_t index,flags;
    uint8_t address[4],netmask[4],broadcast[4],mac[6],configured,reserved[5];
};
_Static_assert(sizeof(struct system_interface_snapshot)==32,"interface syscall snapshot");
struct system_path_request {int32_t id,length;uint64_t buffer;};
_Static_assert(sizeof(struct system_path_request)==16,"system syscall request ABI");
#endif
