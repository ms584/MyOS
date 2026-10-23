"""Production framebuffer bounds/mapping/probe; no privileged instructions run.
The old graphics probe is a negative control for the reported >4GiB gap.
"""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r"""
#include <assert.h>
#include "framebuffer.h"
struct boot_info boot_state;
#include "graphics-source.c"
int main(void){
    uint64_t bytes=3200*600;(void)bytes;
    struct gfx_mode mode;
    boot_state=(struct boot_info){.magic=MYOS_BOOT_MAGIC,.width=800,.height=600,.pitch=3200,.framebuffer=0xe0000000};
    assert(gfx_probe(&mode) && (uintptr_t)mode.pixels==0xe0000000);
    boot_state.framebuffer=0x1001ff000ull;
#ifdef OLD_PROBE
    assert(!gfx_probe(&mode)); /* Old identity-only probe cannot use this valid mode. */
#else
    assert(gfx_probe(&mode));
    assert((uintptr_t)mode.pixels==MYOS_FRAMEBUFFER_WINDOW+0x1ff000);
    boot_state.magic=0;assert(!gfx_probe(&mode)); /* BIOS has no high mapping. */
    boot_state.magic=MYOS_BOOT_MAGIC;
    boot_state.framebuffer=UINT64_MAX-1024;assert(!gfx_probe(&mode));
    boot_state.framebuffer=0x1001ff000ull;
    boot_state.pitch=3196;assert(!gfx_probe(&mode));
    boot_state.pitch=16388;assert(!gfx_probe(&mode));
    boot_state.pitch=3201;assert(!gfx_probe(&mode));
    boot_state.pitch=3200;boot_state.height=0;assert(!gfx_probe(&mode));
    assert(framebuffer_range(0x1001ff000ull,bytes,36));
    assert(!framebuffer_range(0x1001ff000ull,bytes,32));
    assert(!framebuffer_range(0xfffff,bytes,52));
    assert(!framebuffer_range(0x100001,bytes,52));
    assert(!framebuffer_range(0x100000,0,52));
    assert(!framebuffer_range(0x100000,MYOS_FRAMEBUFFER_MAX_BYTES+1,52));
    assert(!framebuffer_range(UINT64_MAX-4,bytes,52));
    assert(!framebuffer_range((1ull<<40)-bytes+4,bytes,40));
    assert(framebuffer_range((1ull<<40)-bytes,bytes,40));
    assert(!framebuffer_range(0x100000,bytes,53));
    assert(!framebuffer_range(0x100000,bytes,31));
    assert(framebuffer_virtual(0xe0000000,bytes)==0xe0000000);
    struct {uint64_t pd[512],guard;} t={.guard=0xabcddcba};
    unsigned n=framebuffer_map(t.pd,0x1001ff000ull,bytes,36);
    assert(n==2 && t.guard==0xabcddcba && !t.pd[n]);
    assert((t.pd[0]&0x000fffffffe00000ull)==0x100000000ull);
    assert((t.pd[1]&0x000fffffffe00000ull)==0x100200000ull);
    for(unsigned i=0;i<n;++i)assert((t.pd[i]&((1ull<<63)|0xfff))==MYOS_FRAMEBUFFER_FLAGS);
    assert(!framebuffer_map(t.pd,UINT64_MAX,bytes,52));
    assert(!framebuffer_map(t.pd,0xe0000000,bytes,52));
    assert(!framebuffer_map(0,0x100000000ull,bytes,52));
    /* 4GiB crossing and the maximum allowed span retain their offset. */
    n=framebuffer_map(t.pd,0xfffff000ull,bytes,36);assert(n==2);
    assert(framebuffer_virtual(0xfffff000ull,bytes)==MYOS_FRAMEBUFFER_WINDOW+0x1ff000);
    n=framebuffer_map(t.pd,0x1001ffffcull,MYOS_FRAMEBUFFER_MAX_BYTES,36);assert(n==9);
    assert(t.guard==0xabcddcba && !t.pd[n]);
#endif
    return 0;
}
"""
def main():
    with tempfile.TemporaryDirectory(prefix='myos-fb-') as directory:
        d=Path(directory);(d/'test.c').write_text(HARNESS)
        for old in (False,True):
            source=(subprocess.check_output(['git','show','413c2e6:drivers/graphics.c'],cwd=ROOT,text=True)
                    if old else (ROOT/'drivers/graphics.c').read_text())
            (d/'graphics-source.c').write_text(source)
            args=['gcc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                  '-ffunction-sections','-fdata-sections','-fsanitize=address,undefined','-fno-builtin','-fno-pie','-no-pie',
                  '-I'+str(ROOT/'include'),'-Wl,--gc-sections',str(d/'test.c'),'-o',str(d/'test')]
            if old:args.insert(1,'-DOLD_PROBE')
            subprocess.run(args,check=True);subprocess.run([str(d/'test')],check=True)
    print('PASS: framebuffer physical bounds/overflow/pitch, low and high probes, UC/NX supervisor mapping, crossing/unaligned pages; old probe rejects valid high mode',flush=True)
if __name__=='__main__':main()
