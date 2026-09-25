#include "browser.h"
#include "mouse.h"
#include "net.h"
#include "widgets.h"

/* Phase 29 preview: bounded HTML text and links over plain HTTP. This is not
 * a browser engine, a CSS renderer, or a replacement for verified TLS. */
#define URL_CAP 192
#define BODY_CAP 32768
#define LINE_CAP 160
#define TEXT_CAP 67
#define HISTORY_CAP 8
#define VISIBLE_LINES 14

struct browser_line {char text[TEXT_CAP+1],href[URL_CAP];uint8_t kind;};
static struct browser_line lines[LINE_CAP];
static unsigned line_count,scroll,history_count,input_length;
static char input[URL_CAP],current[URL_CAP],history[HISTORY_CAP][URL_CAP];
static char title[TEXT_CAP+1],status[72],active_link[URL_CAP];
static bool editing,truncated;

static size_t length(const char *s){size_t n=0;while(s[n])++n;return n;}
static bool begins(const char *s,const char *prefix){while(*prefix)if(*s++!=*prefix++)return false;return true;}
static void copy(char *out,size_t cap,const char *in){size_t i=0;while(i+1<cap&&in[i]){out[i]=in[i];++i;}out[i]=0;}
static void report(const char *message){const char *p="BROWSER: ";while(*p)serial_put(*p++);for(p=message;*p;++p)serial_put(*p);serial_put('\n');}
static void set_status(const char *s){copy(status,sizeof(status),s);report(status);}
static void reset_page(void){memset(lines,0,sizeof(lines));line_count=1;scroll=0;truncated=false;title[0]=0;active_link[0]=0;}
static void new_line(void){
    if(!lines[line_count-1].text[0])return;
    if(line_count<LINE_CAP){++line_count;return;}
    truncated=true;
}
static void append(char c,uint8_t kind){
    if(c=='\r')return;
    if(c=='\n'){new_line();return;}
    if(c=='\t')c=' ';
    if((unsigned char)c<32)c=' ';
    if((unsigned char)c>126)c='?';
    struct browser_line *line=&lines[line_count-1];size_t used=length(line->text);
    if(c==' ' && (!used||line->text[used-1]==' '))return;
    if(used>=TEXT_CAP){new_line();line=&lines[line_count-1];used=length(line->text);if(used>=TEXT_CAP){truncated=true;return;}}
    line->text[used++]=c;line->text[used]=0;
    if(kind>line->kind)line->kind=kind;
    if(active_link[0])copy(line->href,sizeof(line->href),active_link);
}
static bool tag_name(const char *tag,const char *name){
    while(*name){char a=*tag++,b=*name++;if(a>='A'&&a<='Z')a=(char)(a+32);if(a!=b)return false;}
    return !*tag||*tag==' '||*tag=='\t'||*tag=='/'||*tag=='>';
}
static void href_from_tag(const char *tag,char *out){
    out[0]=0;
    size_t length_tag=length(tag);
    for(size_t i=0;i+3<length_tag;++i){
        char a=tag[i],b=tag[i+1],c=tag[i+2],d=tag[i+3];
        if((a=='h'||a=='H')&&(b=='r'||b=='R')&&(c=='e'||c=='E')&&(d=='f'||d=='F')){
            i+=4;while(tag[i]==' '||tag[i]=='\t')++i;if(tag[i]!='=')continue;++i;
            while(tag[i]==' '||tag[i]=='\t')++i;char quote=0;if(tag[i]=='\''||tag[i]=='"')quote=tag[i++];
            size_t used=0;while(tag[i]&&used+1<URL_CAP){
                if((quote&&tag[i]==quote)||(!quote&&(tag[i]==' '||tag[i]=='\t'||tag[i]=='>')))break;
                out[used++]=tag[i++];
            }
            out[used]=0;return;
        }
    }
}
static char entity(const char *s,size_t n){
    if(n==3&&s[0]=='a'&&s[1]=='m'&&s[2]=='p')return '&';
    if(n==2&&s[0]=='l'&&s[1]=='t')return '<';
    if(n==2&&s[0]=='g'&&s[1]=='t')return '>';
    if(n==4&&s[0]=='q'&&s[1]=='u'&&s[2]=='o'&&s[3]=='t')return '"';
    if(n==3&&s[0]=='#'&&s[1]=='3'&&s[2]=='9')return '\'';
    if(n==4&&s[0]=='n'&&s[1]=='b'&&s[2]=='s'&&s[3]=='p')return ' ';
    return 0;
}
static void parse_html(const uint8_t *data,size_t n){
    bool head=false,in_title=false,script=false,style=false,heading=false;
    for(size_t i=0;i<n;){
        if(data[i]=='<' && i+1<n){
            if(i+3<n&&data[i+1]=='!'&&data[i+2]=='-'&&data[i+3]=='-'){
                i+=4;while(i+2<n&&!(data[i]=='-'&&data[i+1]=='-'&&data[i+2]=='>'))++i;
                i=i+2<n?i+3:n;continue;
            }
            size_t start=++i;char quote=0;
            while(i<n){char c=(char)data[i];if(quote){if(c==quote)quote=0;}else if(c=='\''||c=='"')quote=c;else if(c=='>')break;++i;}
            size_t span=i-start;if(i<n)++i;if(span>=256)continue;
            char tag[256];memcpy(tag,data+start,span);tag[span]=0;
            bool closing=tag[0]=='/';const char *name=tag+closing;
            if(tag_name(name,"head")){head=!closing;continue;}
            if(tag_name(name,"title")){in_title=!closing;continue;}
            if(tag_name(name,"script")){script=!closing;continue;}
            if(tag_name(name,"style")){style=!closing;continue;}
            if(tag_name(name,"body")){head=false;continue;}
            if(script||style||head)continue;
            if(tag_name(name,"a")){
                if(closing){active_link[0]=0;new_line();}
                else{new_line();href_from_tag(tag,active_link);}
            }else if(tag_name(name,"br")||tag_name(name,"p")||tag_name(name,"div")||
                     tag_name(name,"li")||tag_name(name,"h1")||tag_name(name,"h2")||tag_name(name,"h3")){
                new_line();if(name[0]=='h')heading=!closing;
            }
            continue;
        }
        if(script||style||(head&&!in_title)){++i;continue;}
        char c=(char)data[i++];
        if(c=='&'){size_t begin=i;while(i<n&&i-begin<8&&data[i]!=';')++i;
            if(i<n&&data[i]==';'){
                char decoded=entity((const char *)data+begin,i-begin);
                if(decoded){c=decoded;++i;}else i=begin;
            }else i=begin;
        }
        if(in_title){size_t used=length(title);if(used<TEXT_CAP && (unsigned char)c>=32){title[used]=c;title[used+1]=0;}continue;}
        if(c=='\n'||c=='\r'||c=='\t')c=' ';
        append(c,active_link[0]?2:(heading?1:0));
    }
    if(!title[0])copy(title,sizeof(title),"Untitled page");
}
static void parse_text(const uint8_t *data,size_t n){
    copy(title,sizeof(title),"Plain text");
    for(size_t i=0;i<n;++i)append((char)data[i],0);
}
static bool collect(void *ctx,const uint8_t *data,size_t n){
    struct {uint8_t *bytes;size_t used;} *buffer=ctx;
    if(n>BODY_CAP-buffer->used)return false;
    memcpy(buffer->bytes+buffer->used,data,n);buffer->used+=n;return true;
}
static bool parse_url(const char *url,char host[URL_CAP],uint16_t *port,char path[URL_CAP]){
    if(begins(url,"https://")){set_status("HTTPS browser integration pending");return false;}
    const char *p=begins(url,"http://")?url+7:url;
    if(!*p){set_status("Enter a URL");return false;}
    for(const char *q=p;*q;++q)if(*q==' '||*q=='\t'||*q=='\r'||*q=='\n'){
        set_status("Online search integration pending");return false;
    }
    if(!begins(url,"http://") && !begins(p,"localhost")){
        bool dot=false,digit=true;for(const char *q=p;*q&&*q!='/'&&*q!=':';++q){if(*q=='.')dot=true;if(*q<'0'||*q>'9')digit=false;}
        if(!dot&&!digit){set_status("Online search integration pending");return false;}
    }
    size_t h=0;while(p[h]&&p[h]!=':'&&p[h]!='/'&&h+1<URL_CAP){host[h]=p[h];++h;}
    host[h]=0;if(!h||h+1==URL_CAP){set_status("Invalid host");return false;}
    p+=h;*port=80;
    if(*p==':'){
        ++p;unsigned value=0,digits=0;
        while(*p>='0'&&*p<='9'&&digits<5){value=value*10+(unsigned)(*p-'0');++p;++digits;}
        if(!digits||value==0||value>65535||(*p&&*p!='/')){set_status("Invalid port");return false;}
        *port=(uint16_t)value;
    }
    if(!*p){copy(path,URL_CAP,"/");return true;}
    if(*p!='/'){set_status("Invalid URL");return false;}
    if(length(p)>=URL_CAP){set_status("Path too long");return false;}
    copy(path,URL_CAP,p);return true;
}
static void loaded(const uint8_t *data,size_t n){
    reset_page();
    size_t body=0;bool found=false;
    while(body+3<n){if(data[body]=='\r'&&data[body+1]=='\n'&&data[body+2]=='\r'&&data[body+3]=='\n'){body+=4;found=true;break;}++body;}
    if(!found){set_status("Malformed HTTP headers");return;}
    bool html=false;for(size_t i=0;i+9<body;++i){
        if(data[i]=='t'&&data[i+1]=='e'&&data[i+2]=='x'&&data[i+3]=='t'&&data[i+4]=='/'&&
           data[i+5]=='h'&&data[i+6]=='t'&&data[i+7]=='m'&&data[i+8]=='l')html=true;
    }
    if(html)parse_html(data+body,n-body);else parse_text(data+body,n-body);
    unsigned code=0;
    if(n>=12&&data[0]=='H'&&data[1]=='T'&&data[2]=='T'&&data[3]=='P'&&data[9]>='0'&&data[9]<='9'&&
       data[10]>='0'&&data[10]<='9'&&data[11]>='0'&&data[11]<='9')
        code=(unsigned)(data[9]-'0')*100u+(unsigned)(data[10]-'0')*10u+(unsigned)(data[11]-'0');
    if(code>=300&&code<400){
        bool secure=false;
        for(size_t i=0;i+18<body;++i)if(data[i]=='L'&&data[i+1]=='o'&&data[i+2]=='c'&&data[i+3]=='a'&&
             data[i+4]=='t'&&data[i+5]=='i'&&data[i+6]=='o'&&data[i+7]=='n'&&data[i+8]==':'){
            size_t j=i+9;while(j<body&&data[j]==' ')++j;
            if(j+8<body&&data[j]=='h'&&data[j+1]=='t'&&data[j+2]=='t'&&data[j+3]=='p'&&
               data[j+4]=='s'&&data[j+5]==':'&&data[j+6]=='/'&&data[j+7]=='/')secure=true;
            break;
        }
        set_status(secure?"HTTPS redirect integration pending":"HTTP redirect not followed");
    }else if(code>=400)set_status("HTTP error response");
    else set_status(truncated?"Page truncated (line limit)":"Page loaded (HTTP, no CSS/JS)");
    report(title);
}
static void navigate(const char *url,bool remember){
    char address[URL_CAP],host[URL_CAP],path[URL_CAP];uint16_t port;
    copy(address,sizeof(address),url);
    if(!parse_url(address,host,&port,path))return;
    if(!begins(address,"http://")){
        char normalized[URL_CAP];size_t n=length(address);
        if(n+7>=sizeof(normalized)){set_status("URL too long");return;}
        copy(normalized,sizeof(normalized),"http://");copy(normalized+7,sizeof(normalized)-7,address);
        copy(address,sizeof(address),normalized);
    }
    if(!net_configured()&&!net_up()){set_status("Network unavailable; run net up");return;}
    uint8_t target[4];if(!net_resolve(host,target)){set_status("DNS failed");return;}
    uint8_t *bytes=kmalloc(BODY_CAP);if(!bytes){set_status("Out of memory");return;}
    struct {uint8_t *bytes;size_t used;} buffer={bytes,0};
    char host_header[URL_CAP];copy(host_header,sizeof(host_header),host);
    if(port!=80){
        size_t used=length(host_header);unsigned value=port;char reversed[5];unsigned count=0;
        do{reversed[count++]=(char)('0'+value%10);value/=10;}while(value);
        if(used+count+2>=sizeof(host_header)){kfree(bytes);set_status("Host name too long");return;}
        host_header[used++]=':';while(count)host_header[used++]=reversed[--count];host_header[used]=0;
    }
    set_status("Loading HTTP page...");
    bool ok=net_http_fetch(target,port,host_header,path,collect,&buffer);
    if(!ok){kfree(bytes);set_status("HTTP failed, timed out or too large");return;}
    if(remember&&current[0]){
        if(history_count==HISTORY_CAP){for(unsigned i=1;i<HISTORY_CAP;++i)copy(history[i-1],URL_CAP,history[i]);--history_count;}
        copy(history[history_count++],URL_CAP,current);
    }
    copy(current,sizeof(current),address);copy(input,sizeof(input),address);input_length=(unsigned)length(input);editing=false;
    loaded(bytes,buffer.used);kfree(bytes);
}
static void back(void){if(!history_count){set_status("No previous page");return;}char url[URL_CAP];copy(url,sizeof(url),history[--history_count]);navigate(url,false);}
static void follow(const char *href){
    char url[URL_CAP];
    if(!href[0]||href[0]=='#'){set_status("Unsupported link");return;}
    for(size_t i=0;href[i]&&href[i]!='/';++i)if(href[i]==':'&&!begins(href,"http://")&&!begins(href,"https://")){
        set_status("Unsupported link");return;
    }
    if(begins(href,"http://")||begins(href,"https://"))copy(url,sizeof(url),href);
    else{
        if(!current[0])return;
        const char *base=begins(current,"http://")?current+7:current;
        size_t prefix=0;while(base[prefix]&&base[prefix]!='/')++prefix;
        size_t head=(size_t)(base-current)+prefix;
        if(href[0]!='/'){
            size_t end=length(current);while(end>head&&current[end-1]!='/')--end;head=end;
        }
        if(head+length(href)+2>=sizeof(url)){set_status("Link too long");return;}
        memcpy(url,current,head);url[head]=0;
        if(href[0]!='/'&&head&&url[head-1]!='/')url[head++]='/';
        copy(url+head,sizeof(url)-head,href);
    }
    navigate(url,true);
}
void browser_begin(void){memset(lines,0,sizeof(lines));input[0]=current[0]=0;input_length=history_count=scroll=0;line_count=1;editing=true;copy(title,sizeof(title),"MyOS Browser preview");set_status("Enter HTTP URL; HTTPS GUI pending");}
void browser_close(void){input[0]=0;editing=false;}
void browser_draw(struct gfx_surface *scene,int x,int y){
    ui_button(scene,(struct ui_rect){x+12,y+42,30,28},"<",0x304b6d,0xffffff);
    const char *shown=input;size_t n=length(input);if(n>60)shown=input+n-60;
    ui_field(scene,(struct ui_rect){x+48,y+42,BROWSER_WIDTH-138,28},shown,0xffffff,0x17263d,editing?0x36a6c3:0x788da5);
    ui_button(scene,(struct ui_rect){x+BROWSER_WIDTH-82,y+42,70,28},"Go",0x267a83,0xffffff);
    gfx_text(scene,x+16,y+80,status,0xdcecf8);
    gfx_rect(scene,x+12,y+105,BROWSER_WIDTH-24,BROWSER_HEIGHT-119,0xf5f7fa);
    gfx_text(scene,x+20,y+110,title,0x172a47);
    for(unsigned row=0;row<VISIBLE_LINES;++row){unsigned at=scroll+row;if(at>=line_count)break;
        uint32_t color=lines[at].kind==2?0x154bb3:lines[at].kind==1?0x172a47:0x222b36;
        gfx_text(scene,x+20,y+134+(int)row*18,lines[at].text,color);
    }
    gfx_text(scene,x+20,y+BROWSER_HEIGHT-31,"Up/Down scroll | Click links | Esc exits Desktop",0x52647d);
}
bool browser_click(int x,int y){
    if(y>=42&&y<70){
        if(x>=12&&x<42){back();return true;}
        if(x>=48&&x<BROWSER_WIDTH-90){editing=true;input[0]=0;input_length=0;return true;}
        if(x>=BROWSER_WIDTH-82&&x<BROWSER_WIDTH-12){navigate(input,true);return true;}
    }
    if(x>=12&&x<BROWSER_WIDTH-12&&y>=134&&y<134+VISIBLE_LINES*18){
        unsigned at=scroll+(unsigned)(y-134)/18;
        if(at<line_count&&lines[at].href[0]){char href[URL_CAP];copy(href,sizeof(href),lines[at].href);follow(href);}
        return true;
    }
    return false;
}
bool browser_key(int key){
    if(key<0||key==27)return false;
    if(key==KEY_UP){if(scroll)--scroll;return true;}
    if(key==KEY_DOWN){if(scroll+VISIBLE_LINES<line_count)++scroll;return true;}
    if(key=='\n'){navigate(input,true);return true;}
    if(key=='\b'){if(!editing){editing=true;return true;}if(input_length){input[--input_length]=0;}return true;}
    if(key>=32&&key<=126){
        if(!editing){input[0]=0;input_length=0;editing=true;}
        if(input_length+1<URL_CAP){input[input_length++]=(char)key;input[input_length]=0;}
        return true;
    }
    return false;
}
