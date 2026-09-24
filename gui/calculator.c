#include "calculator.h"
#include "widgets.h"

/* Values are signed thousandths. Bounding every result keeps multiplication
 * below 10^18, so the freestanding kernel needs no FPU or 128-bit helpers. */
#define CALC_LIMIT 999999999LL

static char entry[16],display[32];
static unsigned length;
static int64_t left;
static char pending;
static bool waiting,result_ready,failed;
static const char *const keys[4][5]={
    {"7","8","9","/","C"},
    {"4","5","6","*","<"},
    {"1","2","3","-","."},
    {"0","","","=","+"}
};

static void copy(char *dest,const char *source)
{
    while((*dest++=*source++));
}
static void report(void)
{
    const char *prefix="CALC: ";
    while(*prefix)serial_put(*prefix++);
    for(const char *p=display;*p;++p)serial_put(*p);
    serial_put('\n');
}
static void error(const char *message)
{
    failed=true;copy(display,message);report();
}
static bool in_range(int64_t value)
{
    return value>=-CALC_LIMIT && value<=CALC_LIMIT;
}
static void format(int64_t value,char *out)
{
    unsigned at=0;
    if(value<0){out[at++]='-';value=-value;}
    uint64_t whole=(uint64_t)value/1000u;
    unsigned fraction=(unsigned)((uint64_t)value%1000u);
    char backwards[12];unsigned count=0;
    do{backwards[count++]=(char)('0'+whole%10u);whole/=10u;}while(whole);
    while(count)out[at++]=backwards[--count];
    if(fraction){
        out[at++]='.';
        out[at++]=(char)('0'+fraction/100u);
        out[at++]=(char)('0'+(fraction/10u)%10u);
        out[at++]=(char)('0'+fraction%10u);
        while(out[at-1]=='0')--at;
    }
    out[at]=0;
}
static bool parse(const char *text,int64_t *value)
{
    bool negative=false,point=false,saw_digit=false;
    unsigned whole_digits=0,fraction_digits=0;
    int64_t whole=0,fraction=0;
    if(*text=='-'){negative=true;++text;}
    for(;*text;++text){
        if(*text=='.' && !point){point=true;continue;}
        if(*text<'0'||*text>'9')return false;
        saw_digit=true;
        if(point){
            if(++fraction_digits>3)return false;
            fraction=fraction*10+(*text-'0');
        }else{
            if(++whole_digits>6)return false;
            whole=whole*10+(*text-'0');
        }
    }
    if(!saw_digit)return false;
    while(fraction_digits++<3)fraction*=10;
    int64_t result=whole*1000+fraction;
    if(!in_range(result))return false;
    *value=negative?-result:result;
    return true;
}
static bool evaluate(int64_t a,int64_t b,char op,int64_t *result)
{
    if(op=='/' && !b){error("Error: divide by zero");return false;}
    switch(op){
    case '+':*result=a+b;break;
    case '-':*result=a-b;break;
    case '*':*result=a*b/1000;break;
    case '/':*result=a*1000/b;break;
    default:error("Error: invalid input");return false;
    }
    if(!in_range(*result)){error("Error: overflow");return false;}
    return true;
}
void calculator_begin(void)
{
    copy(entry,"0");copy(display,"0");length=1;left=0;pending=0;
    waiting=result_ready=failed=false;report();
}
static void digit(char c)
{
    if(result_ready && !pending){calculator_begin();}
    if(waiting){
        entry[0]='0';entry[1]=0;length=1;waiting=false;
    }
    if(c=='.'){
        for(unsigned i=0;i<length;++i)
            if(entry[i]=='.'){error("Error: invalid input");return;}
        entry[length++]='.';entry[length]=0;
    }else{
        if(length==1 && entry[0]=='0')entry[0]=c;
        else{entry[length++]=c;entry[length]=0;}
    }
    int64_t ignored;
    if(!parse(entry,&ignored)){error("Error: invalid input");return;}
    copy(display,entry);result_ready=false;report();
}
static void operation(char op)
{
    if(waiting && pending){pending=op;report();return;}
    int64_t value;
    if(!parse(entry,&value)){error("Error: invalid input");return;}
    if(pending){if(!evaluate(left,value,pending,&left))return;}
    else left=value;
    pending=op;waiting=true;result_ready=false;
    format(left,display);report();
}
static void equals(void)
{
    if(!pending){report();return;}
    if(waiting){error("Error: invalid input");return;}
    int64_t value,result;
    if(!parse(entry,&value)){error("Error: invalid input");return;}
    if(!evaluate(left,value,pending,&result))return;
    left=result;pending=0;waiting=false;result_ready=true;
    format(result,entry);copy(display,entry);
    length=0;while(entry[length])++length;
    report();
}
static void backspace(void)
{
    if(waiting){error("Error: invalid input");return;}
    if(result_ready){calculator_begin();return;}
    if(length<=1){copy(entry,"0");length=1;}
    else entry[--length]=0;
    copy(display,entry);report();
}
bool calculator_key(int key)
{
    if(key<0 || key==27 || key=='q' || key=='Q')return false;
    if(key=='c'||key=='C'){calculator_begin();return true;}
    if(failed)return true; /* Clear is the explicit recovery path. */
    if(key>='0'&&key<='9'){digit((char)key);return true;}
    if(key=='.'){digit('.');return true;}
    if(key=='+'||key=='-'||key=='*'||key=='/'){
        operation((char)key);return true;
    }
    if(key=='='||key=='\n'){equals();return true;}
    if(key=='\b'){backspace();return true;}
    if(key>=32&&key<=126){error("Error: invalid input");return true;}
    return false;
}
void calculator_click(int x,int y)
{
    for(int row=0;row<4;++row)for(int col=0;col<5;++col){
        struct ui_rect r={18+col*56,75+row*32,52,26};
        if(!keys[row][col][0] || !ui_hit(r,x,y))continue;
        int key=keys[row][col][0]=='<'?'\b':keys[row][col][0];
        (void)calculator_key(key);return;
    }
}
void calculator_draw(struct gfx_surface *s,int x,int y)
{
    gfx_rect(s,x+2,y+32,316,176,0x20324b);
    ui_field(s,(struct ui_rect){x+18,y+38,284,32},display,
             0x111a2c,0xffffff,failed?0xd65c69:0x97a9bd);
    for(int row=0;row<4;++row)for(int col=0;col<5;++col){
        const char *label=keys[row][col];
        if(!label[0])continue;
        uint32_t fill=col==3||label[0]=='+'?0x276b8a:0x3a506d;
        if(label[0]=='C'||label[0]=='<')fill=0x83566f;
        ui_button(s,(struct ui_rect){x+18+col*56,y+75+row*32,52,26},
                  label,fill,0xffffff);
    }
}
