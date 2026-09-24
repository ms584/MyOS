#ifndef MYOS_BOOT_H
#define MYOS_BOOT_H
#include <stdint.h>
#define MYOS_BOOT_MAGIC 0x31494645534f594dull
/* Loader/kernel ABI v1. The entry stub reads stack at byte offset 16. */
struct boot_info {
    uint64_t magic,root,stack,font,map;
    uint32_t count,width,height,pitch;
    uint64_t framebuffer;
};
_Static_assert(sizeof(struct boot_info)==64,"boot ABI");
extern struct boot_info boot_state;
void boot_init(const struct boot_info *info);
#endif
