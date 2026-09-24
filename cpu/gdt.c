#include "boot.h"
#include "process.h"

/* Long-mode TSS: hardware reads RSP0 on a Ring 3 -> Ring 0 transition. */
struct tss64 {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;
} __attribute__((packed));
struct gdtr { uint16_t limit; uint64_t base; } __attribute__((packed));
static struct tss64 tss;
static uint64_t gdt[8] __attribute__((aligned(16)));
_Static_assert(sizeof(struct tss64) == 104, "64-bit TSS layout");

void tss_set_stack(uintptr_t top) { tss.rsp[0] = top; }

void process_init(void)
{
    uint64_t flags = irq_save();
    gdt[0] = 0;
    gdt[1] = 0x00cf9a000000ffffull; /* Existing protected-mode selector. */
    gdt[2] = 0x00cf92000000ffffull; /* Kernel data: 0x10. */
    gdt[3] = 0x00af9a000000ffffull; /* Kernel code: 0x18. */
    gdt[4] = 0x00cff2000000ffffull; /* User data: 0x23 including RPL. */
    gdt[5] = 0x00affa000000ffffull; /* User code: 0x2b including RPL. */
    uintptr_t base = (uintptr_t)&tss;
    uint64_t limit = sizeof(tss) - 1;
    gdt[6] = (limit & 0xffff) | ((base & 0xffffff) << 16)
             | (0x89ull << 40) | ((limit & 0xf0000) << 32)
             | ((base & 0xff000000) << 32);
    gdt[7] = base >> 32;
    tss.rsp[0] = boot_state.stack;
    /* An offset beyond the TSS limit denies user-mode port I/O. */
    tss.iomap = sizeof(tss);
    struct gdtr descriptor = { sizeof(gdt) - 1, (uintptr_t)gdt };
    __asm__ volatile ("lgdt %0" : : "m"(descriptor) : "memory");
    uint16_t selector = 0x30;
    __asm__ volatile ("ltr %0" : : "r"(selector) : "memory");
    /* No FPU/SIMD context switching yet: deny x87 use with #NM rather than
       allowing one process to observe another task's floating-point state. */
    uint64_t cr0;
    __asm__ volatile ("mov %%cr0,%0" : "=r"(cr0));
    cr0 |= 0x0a; /* TS + MP; the kernel is compiled with -mgeneral-regs-only. */
    __asm__ volatile ("mov %0,%%cr0" : : "r"(cr0) : "memory");
    irq_restore(flags);
}
