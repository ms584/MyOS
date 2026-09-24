#include "settings.h"
#include "widgets.h"
#include "fs.h"
#include "storage.h"

#define SETTINGS_PATH "/etc/settings.cfg"
#define SETTINGS_MAGIC 0x31544553u /* SET1 */
#define SETTINGS_VERSION 1u

struct values {uint8_t theme,background;uint32_t accent;};
struct record {
    uint32_t magic;
    uint16_t version;
    uint8_t theme,background;
    uint32_t accent,checksum;
} __attribute__((packed));
_Static_assert(sizeof(struct record)==16,"settings record size");

static const struct values defaults={0,0,0x276b8a};
static struct values current,draft;
static struct fs_handle handles[FS_HANDLES];
static char color_text[7];
static unsigned color_length;
static bool editing;
static const char *status;

static void marker(const char *text)
{
    while(*text)serial_put(*text++);
    serial_put('\n');
}
static uint32_t checksum(const void *input,size_t bytes)
{
    const uint8_t *p=input;uint32_t value=2166136261u;
    for(size_t i=0;i<bytes;++i){value^=p[i];value*=16777619u;}
    return value;
}
static char digit(unsigned n){return "0123456789ABCDEF"[n&15u];}
static void format_color(uint32_t value)
{
    for(int i=5;i>=0;--i){color_text[i]=digit(value);value>>=4;}
    color_text[6]=0;color_length=6;
}
static int hex_value(char c)
{
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static bool parse_color(uint32_t *value)
{
    if(color_length!=6)return false;
    uint32_t result=0;
    for(unsigned i=0;i<6;++i){int n=hex_value(color_text[i]);if(n<0)return false;result=(result<<4)|(unsigned)n;}
    *value=result;return true;
}
static bool valid(const struct record *r)
{
    return r->magic==SETTINGS_MAGIC && r->version==SETTINGS_VERSION &&
           r->theme<=1 && r->background<=1 && r->accent<=0xffffff &&
           r->checksum==checksum(r,sizeof(*r)-sizeof(r->checksum));
}
void settings_init(void)
{
    current=draft=defaults;
    editing=false;format_color(current.accent);
    status=storage_ready()?"Defaults; not saved":"RAM only; reboot resets";
    struct fs_entry entry;
    int result=fs_stat(SETTINGS_PATH,&entry);
    if(result==-2)return;
    if(result<0 || entry.type!=FS_FILE || entry.size!=sizeof(struct record))goto invalid;
    int fd=fs_open(handles,SETTINGS_PATH,FS_READ);
    if(fd<0)goto invalid;
    struct record record={0};
    int64_t count=fs_read(handles,fd,&record,sizeof(record));
    fs_close(handles,fd);
    if(count!=(int64_t)sizeof(record) || !valid(&record))goto invalid;
    current=(struct values){record.theme,record.background,record.accent};
    draft=current;format_color(current.accent);
    status=storage_ready()?"Restored from disk":"Restored in RAM";
    marker("SETTINGS RESTORED");
    return;
invalid:
    status="Invalid saved settings";
    marker("SETTINGS INVALID FILE");
}
void settings_begin(void)
{
    draft=current;format_color(current.accent);editing=false;
    status=storage_ready()?"Changes need Save":"RAM only; reboot resets";
}
void settings_cancel(void)
{
    draft=current;format_color(current.accent);editing=false;
}
uint32_t settings_background_color(void)
{
    if(draft.theme)return draft.background?0xe4f2e9:0xe6effa;
    return draft.background?0x142821:0x111a2c;
}
uint32_t settings_bar_color(void)
{
    return draft.theme?0xb9cce0:(draft.background?0x23483e:0x24395a);
}
uint32_t settings_text_color(void){return draft.theme?0x142338:0xffffff;}
uint32_t settings_accent_color(void){return draft.accent;}

static bool save(void)
{
    uint32_t accent;
    if(!parse_color(&accent)){
        status="Invalid hex color";marker("SETTINGS INVALID COLOR");return false;
    }
    struct values candidate=draft;candidate.accent=accent;
    struct record record={SETTINGS_MAGIC,SETTINGS_VERSION,candidate.theme,
                          candidate.background,candidate.accent,0};
    record.checksum=checksum(&record,sizeof(record)-sizeof(record.checksum));
    int fd=fs_open(handles,SETTINGS_PATH,FS_WRITE|FS_CREATE|FS_TRUNC);
    if(fd<0){status="Cannot create settings file";marker("SETTINGS SAVE FAILED");return false;}
    int64_t written=fs_write(handles,fd,&record,sizeof(record));
    fs_close(handles,fd);
    if(written!=(int64_t)sizeof(record)){
        status="Cannot write settings file";marker("SETTINGS SAVE FAILED");return false;
    }
    current=draft=candidate;format_color(accent);editing=false;
    if(!storage_ready()){
        status="RAM only; reboot resets";marker("SETTINGS SAVED RAM");return true;
    }
    if(storage_save()<0){
        status="Disk save failed; RAM only";marker("SETTINGS SAVE FAILED");return false;
    }
    status="Saved to disk";marker("SETTINGS SAVED DISK");return true;
}
static struct ui_rect rect(int x,int y,int width,int height)
{
    return (struct ui_rect){x,y,width,height};
}
void settings_click(int x,int y)
{
    editing=false;
    if(ui_hit(rect(18,60,100,24),x,y)){draft.theme=0;status="Unsaved changes";return;}
    if(ui_hit(rect(140,60,100,24),x,y)){draft.theme=1;status="Unsaved changes";return;}
    if(ui_hit(rect(18,114,100,24),x,y)){draft.background=0;status="Unsaved changes";return;}
    if(ui_hit(rect(140,114,100,24),x,y)){draft.background=1;status="Unsaved changes";return;}
    if(ui_hit(rect(18,166,165,24),x,y)){editing=true;status="Enter 6 hex digits";return;}
    if(ui_hit(rect(200,166,100,24),x,y)){(void)save();return;}
}
bool settings_editing(void){return editing;}
bool settings_key(int key)
{
    if(!editing)return false;
    if(key==27){editing=false;status="Edit cancelled";return true;}
    if(key=='\n'){editing=false;(void)save();return true;}
    if(key=='\b'){
        if(color_length)color_text[--color_length]=0;
        status="Unsaved changes";return true;
    }
    if(key>=32 && key<=126){
        if(color_length<6){color_text[color_length++]=(char)key;color_text[color_length]=0;}
        else status="Hex field holds 6 chars";
        return true;
    }
    return false;
}
void settings_draw(struct gfx_surface *s,int x,int y)
{
    bool light=draft.theme;
    uint32_t body=light?0xf4f7fa:0x20324b;
    uint32_t ink=light?0x142338:0xe1ecfa;
    uint32_t normal=light?0xd3e0ec:0x3a506d;
    gfx_rect(s,x+2,y+32,316,176,body);
    ui_label(s,x+18,y+40,"Theme",ink);
    ui_button(s,rect(x+18,y+60,100,24),"Dark",
              draft.theme==0?draft.accent:normal,draft.theme==0?0xffffff:ink);
    ui_button(s,rect(x+140,y+60,100,24),"Light",
              draft.theme==1?draft.accent:normal,draft.theme==1?0xffffff:ink);
    ui_label(s,x+18,y+94,"Background",ink);
    ui_button(s,rect(x+18,y+114,100,24),"Blue",
              draft.background==0?draft.accent:normal,draft.background==0?0xffffff:ink);
    ui_button(s,rect(x+140,y+114,100,24),"Green",
              draft.background==1?draft.accent:normal,draft.background==1?0xffffff:ink);
    ui_label(s,x+18,y+145,"Accent #RRGGBB",ink);
    ui_field(s,rect(x+18,y+166,165,24),color_text,
             light?0xffffff:0x17243b,ink,editing?draft.accent:0x97a9bd);
    ui_button(s,rect(x+200,y+166,100,24),"Save",draft.accent,0xffffff);
    ui_label(s,x+18,y+191,status,ink);
}
void settings_print(void)
{
    print("Settings: theme=");print(current.theme?"light":"dark");
    print(" background=");print(current.background?"green":"blue");
    print(" accent=#");
    for(int i=5;i>=0;--i)putchar(digit(current.accent>>(i*4)));
    print(" persistence=");print(storage_ready()?"disk":"RAM-only");print("\n");
}
