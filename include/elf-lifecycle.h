#ifndef MYOS_ELF_LIFECYCLE_H
#define MYOS_ELF_LIFECYCLE_H
#include <stdint.h>
#define ELF_PAIR_CONTROL_ADDRESS 0x8010011000ull
#define ELF_PAIR_MAGIC 0x4d594f5350414901ull
/* Private MyOS ABI, not glibc's link_map/DTV/exit frame. Order is core, provider;
 * initialization walks backwards, finalization walks forwards. */
struct elf_image_lifecycle {
    uint64_t init,array,count,fini,fini_array,fini_count,cxa,active;
};
struct elf_pair_control {
    uint64_t magic,count,exit_owner,reserved;
    struct elf_image_lifecycle images[2];
};
_Static_assert(sizeof(struct elf_image_lifecycle)==64&&sizeof(struct elf_pair_control)==160,"paired lifecycle ABI");
#endif
