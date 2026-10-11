#include "vmmouse.h"
#define VM_MAGIC 0x564d5868u
#define VM_PORT 0x5658u
static bool enabled;
struct reply { uint32_t a,b,c,d; };
static struct reply request(uint32_t command,uint32_t argument)
{
    uint32_t a=VM_MAGIC,b=argument,c=command,d=VM_PORT,si=0,di=0;
    __asm__ volatile("inl %%dx, %%eax":"+a"(a),"+b"(b),"+c"(c),"+d"(d),
                     "+S"(si),"+D"(di)::"memory");
    return (struct reply){a,b,c,d};
}
bool vmmouse_enabled(void){return enabled;}
void vmmouse_disable(void)
{
    if(enabled)request(41,0xf5u);
    enabled=false;
}
bool vmmouse_enable(void)
{
    if(enabled)return true;
    uint32_t a=1,b,c,d;
    __asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));
    if(!(c&(1u<<31)))return false; /* Never probe the port on bare hardware. */
    struct reply version=request(10,~VM_MAGIC);
    if(version.b!=VM_MAGIC||version.a==UINT32_MAX)return false;
    request(41,0x45414552u); /* Enable and retrieve protocol version. */
    enabled=true;
    uint32_t status=request(40,0).a;
    if((status>>16)||(status&0xffffu)!=1||request(39,1).a!=0x3442554au){
        vmmouse_disable();return false;
    }
    request(41,0x53424152u); /* Absolute coordinates, no host acceleration delta. */
    return true;
}
int vmmouse_read(struct vmmouse_event *event)
{
    if(!enabled||!event)return 0;
    uint32_t status=request(40,0).a,count=status&0xffffu;
    if((status>>16)||count>1024||(count%4)){
        vmmouse_disable();return -1;
    }
    if(!count)return 0;
    struct reply data=request(39,4);
    if((data.a&0x10000u)||data.b>65535||data.c>65535){
        vmmouse_disable();return -1;
    }
    uint8_t buttons=((data.a&0x20)?1:0)|((data.a&0x10)?2:0)|((data.a&8)?4:0);
    *event=(struct vmmouse_event){(uint16_t)data.b,(uint16_t)data.c,buttons};
    return 1;
}
