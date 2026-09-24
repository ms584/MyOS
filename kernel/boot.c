#include "boot.h"
#include "os.h"
struct boot_info boot_state;
void boot_init(const struct boot_info *info)
{
    if(info){
        if(info->magic!=MYOS_BOOT_MAGIC)for(;;)__asm__ volatile("cli; hlt");
        boot_state=*info;
    }else{
        boot_state=(struct boot_info){0};
        boot_state.root=0x1000;boot_state.stack=0x90000;
        boot_state.font=0xa000;boot_state.map=0x7400;
        boot_state.count=*(volatile uint16_t *)0x7000;
        if(*(volatile uint8_t *)0x7004){
            const uint8_t *v=(const uint8_t *)0xb200;
            if(v[25]==32){
                boot_state.width=*(const uint16_t *)(v+18);
                boot_state.height=*(const uint16_t *)(v+20);
                boot_state.pitch=*(const uint16_t *)(v+16);
                boot_state.framebuffer=*(const uint32_t *)(v+40);
            }
        }
    }
}
