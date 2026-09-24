#include "snake.h"
#include "widgets.h"
#include "fs.h"
#include "storage.h"

#define BOARD_WIDTH 20
#define BOARD_HEIGHT 8
#define BOARD_CELLS (BOARD_WIDTH*BOARD_HEIGHT)
#define CELL_SIZE 14
#define STEP_TICKS 25u
#define SCORE_PATH "/home/snake.score"
#define SCORE_MAGIC 0x314b4e53u /* SNK1 */
#define SCORE_VERSION 1u

struct point {uint8_t x,y;};
struct score_record {
    uint32_t magic;
    uint16_t version,high;
    uint32_t checksum;
} __attribute__((packed));
_Static_assert(sizeof(struct score_record)==12,"Snake score record size");

enum { RIGHT,DOWN,LEFT,UP };
static struct point body[BOARD_CELLS],food;
static unsigned length,score,high,direction,queued;
static uint32_t random_state;
static uint64_t last_tick;
static bool turn_queued,paused,dead,won,active,save_failed;
static struct fs_handle handles[FS_HANDLES];

static void marker(const char *name,unsigned value)
{
    const char *p="SNAKE ";while(*p)serial_put(*p++);
    while(*name)serial_put(*name++);
    serial_put(' ');
    char digits[12];unsigned n=0;
    do{digits[n++]=(char)('0'+value%10u);value/=10u;}while(value);
    while(n)serial_put(digits[--n]);
    serial_put('\n');
}
static bool same(struct point a,struct point b)
{
    return a.x==b.x && a.y==b.y;
}
static uint32_t checksum(const void *input,size_t bytes)
{
    const uint8_t *p=input;uint32_t value=2166136261u;
    for(size_t i=0;i<bytes;++i){value^=p[i];value*=16777619u;}
    return value;
}
static bool valid(const struct score_record *r)
{
    return r->magic==SCORE_MAGIC && r->version==SCORE_VERSION &&
           r->high<=BOARD_CELLS-3 &&
           r->checksum==checksum(r,sizeof(*r)-sizeof(r->checksum));
}
void snake_init(void)
{
    high=0;active=false;save_failed=false;
    struct fs_entry entry;
    int result=fs_stat(SCORE_PATH,&entry);
    if(result==-2)return;
    if(result<0 || entry.type!=FS_FILE || entry.size!=sizeof(struct score_record))goto invalid;
    int fd=fs_open(handles,SCORE_PATH,FS_READ);
    if(fd<0)goto invalid;
    struct score_record r={0};
    int64_t count=fs_read(handles,fd,&r,sizeof(r));
    fs_close(handles,fd);
    if(count!=(int64_t)sizeof(r) || !valid(&r))goto invalid;
    high=r.high;marker("HIGH RESTORED",high);return;
invalid:
    marker("HIGH INVALID",0);
}
static void save_high(void)
{
    struct score_record r={SCORE_MAGIC,SCORE_VERSION,(uint16_t)high,0};
    r.checksum=checksum(&r,sizeof(r)-sizeof(r.checksum));
    int fd=fs_open(handles,SCORE_PATH,FS_WRITE|FS_CREATE|FS_TRUNC);
    if(fd<0){save_failed=true;marker("HIGH SAVE FAILED",high);return;}
    int64_t count=fs_write(handles,fd,&r,sizeof(r));
    fs_close(handles,fd);
    if(count!=(int64_t)sizeof(r)){save_failed=true;marker("HIGH SAVE FAILED",high);return;}
    if(!storage_ready()){marker("HIGH RAM",high);return;}
    if(storage_save()<0){save_failed=true;marker("HIGH SAVE FAILED",high);return;}
    save_failed=false;marker("HIGH SAVED",high);
}
static bool occupied(struct point p)
{
    for(unsigned i=0;i<length;++i)if(same(body[i],p))return true;
    return false;
}
static bool place_food(void)
{
    random_state=random_state*1664525u+1013904223u;
    unsigned start=random_state%BOARD_CELLS;
    for(unsigned i=0;i<BOARD_CELLS;++i){
        unsigned cell=(start+i)%BOARD_CELLS;
        struct point candidate={(uint8_t)(cell%BOARD_WIDTH),(uint8_t)(cell/BOARD_WIDTH)};
        if(!occupied(candidate)){food=candidate;return true;}
    }
    return false;
}
void snake_begin(void)
{
    length=3;score=0;direction=queued=RIGHT;turn_queued=false;
    paused=dead=won=false;active=true;last_tick=ticks;
    body[0]=(struct point){5,3};body[1]=(struct point){4,3};body[2]=(struct point){3,3};
    food=(struct point){8,3};
    random_state=(uint32_t)ticks^0x9e3779b9u;
    marker("START",high);
}
static void step(void)
{
    direction=queued;turn_queued=false;
    int x=body[0].x,y=body[0].y;
    if(direction==RIGHT)++x;
    else if(direction==LEFT)--x;
    else if(direction==DOWN)++y;
    else --y;
    if(x<0||x>=BOARD_WIDTH||y<0||y>=BOARD_HEIGHT){
        dead=true;marker("GAME OVER",score);return;
    }
    struct point next={(uint8_t)x,(uint8_t)y};
    bool eating=same(next,food);
    unsigned count=length-(eating?0u:1u);
    for(unsigned i=0;i<count;++i)if(same(next,body[i])){
        dead=true;marker("GAME OVER",score);return;
    }
    if(eating)++length;
    for(unsigned i=length-1;i>0;--i)body[i]=body[i-1];
    body[0]=next;
    marker("STEP",(unsigned)y*BOARD_WIDTH+(unsigned)x);
    if(eating){
        ++score;marker("SCORE",score);
        if(score>high){high=score;save_high();}
        if(!place_food()){won=true;marker("WIN",score);}
    }
}
bool snake_update(uint64_t now)
{
    if(!active || paused || dead || won)return false;
    if(now-last_tick<STEP_TICKS)return false;
    last_tick=now;step();return true;
}
static void pause_toggle(void)
{
    if(dead||won)return;
    paused=!paused;last_tick=ticks;
    marker(paused?"PAUSE":"RESUME",score);
}
bool snake_key(int key)
{
    if(!active)return false;
    if(key=='p'||key=='P'||key==' '){pause_toggle();return true;}
    if(key=='r'||key=='R'){snake_begin();marker("RESTART",score);return true;}
    unsigned next;
    if(key==KEY_RIGHT||key=='d'||key=='D')next=RIGHT;
    else if(key==KEY_DOWN||key=='s'||key=='S')next=DOWN;
    else if(key==KEY_LEFT||key=='a'||key=='A')next=LEFT;
    else if(key==KEY_UP||key=='w'||key=='W')next=UP;
    else return false;
    if(!dead&&!won&&!turn_queued && (next+2u)%4u!=direction){
        queued=next;turn_queued=true;
        marker("TURN",next);
    }
    return true;
}
void snake_click(int x,int y)
{
    if(ui_hit((struct ui_rect){20,180,96,24},x,y))pause_toggle();
    else if(ui_hit((struct ui_rect){204,180,96,24},x,y)){
        snake_begin();marker("RESTART",score);
    }
}
static void append_number(char *text,unsigned *at,unsigned value)
{
    char digits[12];unsigned n=0;
    do{digits[n++]=(char)('0'+value%10u);value/=10u;}while(value);
    while(n)text[(*at)++]=digits[--n];
}
void snake_draw(struct gfx_surface *s,int x,int y)
{
    gfx_rect(s,x+2,y+32,316,176,0x20324b);
    char line[48];unsigned at=0;
    const char *prefix="Score ";while(*prefix)line[at++]=*prefix++;
    append_number(line,&at,score);
    prefix="  Best ";while(*prefix)line[at++]=*prefix++;
    append_number(line,&at,high);
    prefix=save_failed?"  Save failed":storage_ready()?"  Disk":"  RAM only";
    while(*prefix)line[at++]=*prefix++;
    line[at]=0;
    ui_label(s,x+20,y+38,line,0xe1ecfa);
    gfx_rect(s,x+18,y+58,284,116,0x8299af);
    gfx_rect(s,x+20,y+60,BOARD_WIDTH*CELL_SIZE,BOARD_HEIGHT*CELL_SIZE,0x111a2c);
    for(unsigned i=0;i<length;++i)
        gfx_rect(s,x+20+body[i].x*CELL_SIZE+1,y+60+body[i].y*CELL_SIZE+1,
                 CELL_SIZE-2,CELL_SIZE-2,i?0x48ba75:0x92e46b);
    if(!won)gfx_rect(s,x+20+food.x*CELL_SIZE+2,y+60+food.y*CELL_SIZE+2,
                     CELL_SIZE-4,CELL_SIZE-4,0xf0a34e);
    ui_button(s,(struct ui_rect){x+20,y+180,96,24},paused?"Resume":"Pause",
              0x3a506d,0xffffff);
    ui_button(s,(struct ui_rect){x+204,y+180,96,24},"Restart",0x3a506d,0xffffff);
    ui_label(s,x+122,y+184,dead?"Game over":won?"You won":paused?"Paused":"Playing",0xffffff);
}
