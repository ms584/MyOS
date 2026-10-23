#ifndef MYOS_BOOT_H
#define MYOS_BOOT_H
#include <stdint.h>
#define MYOS_BOOT_MAGIC 0x32494645534f594dull
/* One reserved supervisor PDPT slot, shared by kernel and process CR3 roots. */
#define MYOS_FRAMEBUFFER_WINDOW 0x7e00000000ull
/* BIOS low-memory ownership; UEFI allocates owned stack/DMA pages separately.
 * The BIOS initial stack grows down, DMA grows upward outside kernel/BSS. */
#define MYOS_KERNEL_LIMIT 0x80000u
#define MYOS_INITIAL_STACK_BOTTOM 0x80000u
#define MYOS_INITIAL_STACK_TOP 0x90000u
#define MYOS_SB16_DMA_BASE 0x90000u
#define MYOS_SB16_DMA_BYTES 16384u
#define MYOS_INITIAL_STACK_BYTES (MYOS_INITIAL_STACK_TOP-MYOS_INITIAL_STACK_BOTTOM)
_Static_assert(MYOS_KERNEL_LIMIT<=MYOS_INITIAL_STACK_BOTTOM,"kernel/stack overlap");
_Static_assert(MYOS_SB16_DMA_BASE>=MYOS_INITIAL_STACK_TOP,"stack/DMA overlap");
_Static_assert((MYOS_SB16_DMA_BASE&0xffffu)+MYOS_SB16_DMA_BYTES<=0x10000u,"DMA boundary");
/* Loader/kernel ABI v2 adds owned ISA DMA at offset 64. Stack stays at 16. */
struct boot_info {
    uint64_t magic,root,stack,font,map;
    uint32_t count,width,height,pitch;
    uint64_t framebuffer;
    uint64_t dma;
};
_Static_assert(sizeof(struct boot_info)==72,"boot ABI");
extern struct boot_info boot_state;
void boot_init(const struct boot_info *info);
#endif
