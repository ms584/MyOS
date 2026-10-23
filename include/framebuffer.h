#ifndef MYOS_FRAMEBUFFER_H
#define MYOS_FRAMEBUFFER_H
#include "boot.h"
/* Bound direct GOP access to the selected mode, not the entire GPU aperture.
 * Physical address width comes from CPUID in the loader. No 32-bit truncation. */
#define MYOS_FRAMEBUFFER_MAX_BYTES (16ull*1024*1024)
#define MYOS_FRAMEBUFFER_PAGE 0x200000ull
#define MYOS_FRAMEBUFFER_FLAGS ((1ull<<63)|0x9bull) /* supervisor RW, UC, NX, large */
static inline int framebuffer_range(uint64_t base,uint64_t bytes,unsigned bits){
    return bits>=32 && bits<=52 && base>=0x100000 && !(base&3) && bytes &&
        bytes<=MYOS_FRAMEBUFFER_MAX_BYTES && base<=(1ull<<bits)-bytes;
}
static inline int framebuffer_high(uint64_t base,uint64_t bytes){
    return base+bytes>0x100000000ull; /* Call only after framebuffer_range. */
}
static inline uint64_t framebuffer_virtual(uint64_t base,uint64_t bytes){
    return framebuffer_high(base,bytes)?MYOS_FRAMEBUFFER_WINDOW+(base&(MYOS_FRAMEBUFFER_PAGE-1)):base;
}
/* Dedicated zeroed page directory. At most nine large pages are needed. */
static inline unsigned framebuffer_map(uint64_t *pd,uint64_t base,uint64_t bytes,unsigned bits){
    if(!pd || !framebuffer_range(base,bytes,bits) || !framebuffer_high(base,bytes))return 0;
    uint64_t aligned=base&~(MYOS_FRAMEBUFFER_PAGE-1);
    unsigned count=(unsigned)((base-aligned+bytes+MYOS_FRAMEBUFFER_PAGE-1)/MYOS_FRAMEBUFFER_PAGE);
    for(unsigned i=0;i<count;++i)pd[i]=(aligned+(uint64_t)i*MYOS_FRAMEBUFFER_PAGE)|MYOS_FRAMEBUFFER_FLAGS;
    return count;
}
#endif
