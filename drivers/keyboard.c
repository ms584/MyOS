#include "os.h"
#include "native-ui.h"
static volatile unsigned head,tail;
static uint8_t buffer[128];
static bool left_shift,right_shift,caps,caps_down,extended;
static bool left_control,right_control,left_alt,right_alt,left_meta,right_meta;
static unsigned pause_bytes;
static const char plain[128]={
 [1]=27,[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]='\b',
 [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',
 [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'', [41]='`',
 [43]='\\',[44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
static const char shifted[128]={
 [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',
 [26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
static void enqueue(uint8_t c){
    unsigned next=(head+1)%sizeof(buffer);if(next==tail)return;
    buffer[head]=c;__asm__ volatile("":::"memory");head=next;
}
static bool input_ready(void){for(unsigned n=0;n<100000;++n)if(!(inb(0x64)&2))return true;return false;}
static bool output_ready(void){for(unsigned n=0;n<100000;++n)if(inb(0x64)&1)return true;return false;}
static void command(uint8_t v){check(input_ready(),"PS/2 controller timeout");outb(0x64,v);}
void keyboard_init(void){
    command(0xad);command(0xa7);
    for(unsigned n=0;n<256 && (inb(0x64)&1);++n)(void)inb(0x60);
    command(0x20);check(output_ready(),"PS/2 config timeout");uint8_t cfg=inb(0x60);
    command(0x60);check(input_ready(),"PS/2 config write");outb(0x60,(cfg|0x41)&~0x12);
    command(0xae);check(input_ready(),"PS/2 enable timeout");outb(0x60,0xf4);
    check(output_ready(),"PS/2 ACK timeout");check(inb(0x60)==0xfa,"PS/2 scanning ACK");
    outb(0x21,inb(0x21)&~2u);
}
void keyboard_irq(void){
    uint8_t status=inb(0x64);if(!(status&1))return;
    uint8_t code=inb(0x60);if(status&0xe0)return;
    if(pause_bytes){--pause_bytes;return;}
    if(code==0xe1){pause_bytes=5;return;}
    if(code==0xe0){extended=true;return;}
    bool was_extended=extended;extended=false;
    bool release=code&0x80;code&=0x7f;
    if(!was_extended&&code==42)left_shift=!release;
    if(!was_extended&&code==54)right_shift=!release;
    if(code==29){if(was_extended)right_control=!release;else left_control=!release;}
    if(code==56){if(was_extended)right_alt=!release;else left_alt=!release;}
    if(was_extended&&code==91)left_meta=!release;
    if(was_extended&&code==92)right_meta=!release;
    if(!was_extended&&code==58){if(!release&&!caps_down)caps=!caps;caps_down=!release;}
    uint32_t modifiers=(left_shift||right_shift?NATIVE_UI_SHIFT:0)|
        (left_control||right_control?NATIVE_UI_CONTROL:0)|(left_alt||right_alt?NATIVE_UI_ALT:0)|
        (left_meta||right_meta?NATIVE_UI_META:0)|(caps?NATIVE_UI_CAPS:0);
    native_ui_set_modifiers(modifiers);
    char c=was_extended?0:plain[code];
    bool shift=left_shift||right_shift;
    if(c>='a'&&c<='z'){if(shift!=caps)c-=32;}else if(shift&&shifted[code])c=shifted[code];
    struct native_ui_event event={release?NATIVE_UI_KEY_UP:NATIVE_UI_KEY_DOWN,
        code|(was_extended?0x100u:0),release?0:(uint8_t)c,modifiers,0,0,0,0,0,0,0};
    native_ui_emit(&event);
    if(release)return;
    if(was_extended){
        int key=code==0x48?KEY_UP:code==0x50?KEY_DOWN:
                code==0x4b?KEY_LEFT:code==0x4d?KEY_RIGHT:0;
        if(key)enqueue((uint8_t)key);return;
    }
    if(c)enqueue((uint8_t)c);
}
int keyboard_get(void){uint64_t f=irq_save();int c=-1;if(tail!=head){c=buffer[tail];tail=(tail+1)%sizeof(buffer);}irq_restore(f);return c;}
