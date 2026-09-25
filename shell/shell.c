#include "app.h"
#include "fs.h"
#include "storage.h"
#include "graphics.h"
#include "mouse.h"
#include "desktop.h"
#include "settings.h"
#define LINE_SIZE 256
static char cwd[FS_PATH]="/";
static struct fs_handle files[FS_HANDLES];
static bool equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static size_t length(const char *s){size_t n=0;while(s[n])++n;return n;}
static char *word(char **rest){char *p=*rest;while(*p==' ')++p;if(!*p){*rest=p;return 0;}char *start=p;while(*p&&*p!=' ')++p;if(*p)*p++=0;*rest=p;return start;}
static void error(int code){
    const char *s="operation failed";
    switch(code){case -2:s="not found";break;case -8:s="invalid executable";break;case -110:s="app timed out";break;case -9:s="bad file handle";break;case -12:s="out of memory";break;case -16:s="busy";break;case -17:s="already exists";break;case -20:s="not a directory";break;case -21:s="is a directory";break;case -22:s="invalid path or argument";break;case -24:s="too many open files";break;case -28:s="filesystem full";break;case -36:s="path or name too long";break;case -39:s="directory not empty";break;}
    print("error: ");print(s);print("\n");
}
static bool path_for(const char *arg,char *out){
    size_t a=length(arg),base=arg[0]=='/'?0:length(cwd);
    bool slash=base&&cwd[base-1]!='/';
    if(base+slash+a>=FS_PATH){error(-36);return false;}
    memcpy(out,cwd,base);if(slash)out[base++]='/';memcpy(out+base,arg,a+1);return true;
}
/* Only normalize after filesystem resolution has validated intermediate nodes. */
static void canonical(const char *path,char *out){
    size_t used=1,pos=0;out[0]='/';out[1]=0;
    while(path[pos]){
        while(path[pos]=='/')++pos;size_t begin=pos;while(path[pos]&&path[pos]!='/')++pos;size_t n=pos-begin;
        if(!n)break;if(n==1&&path[begin]=='.')continue;
        if(n==2&&path[begin]=='.'&&path[begin+1]=='.'){while(used>1&&out[used-1]!='/')--used;if(used>1)--used;out[used]=0;continue;}
        if(used>1)out[used++]='/';memcpy(out+used,path+begin,n);used+=n;out[used]=0;
    }
}
static void persist(void){
    if(!storage_ready())return;
    int result=storage_save();
    if(result<0){print("error: disk save failed (code ");decimal((uint64_t)(-result));print(")\n");}
}
static void execute(char *line){
    char *rest=line,*cmd=word(&rest);if(!cmd)return;
    if(equal(cmd,"echo")){while(*rest==' ')++rest;print(rest);print("\n");return;}
    char *arg=word(&rest);
    if(equal(cmd,"gfx")){if(arg || word(&rest))print("error: unexpected argument\n");else gfx_demo();return;}
    if(equal(cmd,"desktop")){if(arg || word(&rest))print("error: unexpected argument\n");else desktop_run();return;}
    if(equal(cmd,"settings")){if(arg || word(&rest))print("error: unexpected argument\n");else settings_print();return;}
    if(equal(cmd,"mouse")){
        if(arg || word(&rest)){print("error: unexpected argument\n");return;}
        if(!mouse_ready()){print("Mouse unavailable\n");return;}
        int x,y;uint8_t buttons;mouse_position(&x,&y,&buttons);
        print("Mouse: x=");decimal((uint64_t)x);print(" y=");decimal((uint64_t)y);
        print(" buttons=");decimal(buttons);print(" dropped=");decimal(mouse_dropped());print("\n");
        return;
    }
    if(equal(cmd,"run")){
        if(!arg || word(&rest)){print("usage: run PATH\n");return;}
        char path[FS_PATH];if(!path_for(arg,path))return;
        int32_t status=0;int result=app_run(path,&status);
        if(result<0)error(result);
        else{print("App exited with status ");decimal((uint32_t)status);print("\n");}
        return;
    }
    if(equal(cmd,"cp")){
        char *destination=word(&rest);
        if(!arg||!destination||word(&rest)){print("usage: cp SOURCE DEST\n");return;}
        char source_path[FS_PATH],destination_path[FS_PATH];
        if(!path_for(arg,source_path)||!path_for(destination,destination_path))return;
        struct fs_entry info;int result=fs_stat(source_path,&info);
        if(result<0){error(result);return;}
        if(info.type!=FS_FILE){error(-21);return;}
        result=fs_stat(destination_path,&info);
        if(result!=-2){error(result<0?result:-17);return;}
        char *buffer=kmalloc(FS_FILE_MAX);
        if(!buffer){error(-12);return;}
        int from=fs_open(files,source_path,FS_READ),to=-1;
        if(from>=3){
            int64_t length=fs_read(files,from,buffer,FS_FILE_MAX);
            if(length>=0){
                to=fs_open(files,destination_path,FS_WRITE|FS_CREATE);
                result=to<0?to:(fs_write(files,to,buffer,(size_t)length)==length?0:-28);
            }else result=(int)length;
            (void)fs_close(files,from);
        }else result=from;
        if(to>=3)(void)fs_close(files,to);
        kfree(buffer);
        if(result<0)error(result);else persist();
        return;
    }
    if(equal(cmd,"write")){
        if(!arg){print("usage: write PATH TEXT\n");return;}
        while(*rest==' ')++rest;
        char path[FS_PATH];if(!path_for(arg,path))return;
        int fd=fs_open(files,path,FS_WRITE|FS_CREATE|FS_TRUNC);if(fd<0){error(fd);return;}
        int64_t result=fs_write(files,fd,rest,length(rest));fs_close(files,fd);if(result<0)error((int)result);else persist();return;
    }
    if(word(&rest)){print("error: too many arguments\n");return;}
    if(equal(cmd,"help")||equal(cmd,"clear")||equal(cmd,"pwd")||equal(cmd,"mem")||equal(cmd,"uptime")||equal(cmd,"disk")||equal(cmd,"sync")){
        if(arg){print("error: unexpected argument\n");return;}
        if(equal(cmd,"help"))print("help clear gfx desktop settings mouse echo TEXT pwd cd [PATH] ls [PATH]\ncat PATH touch PATH write PATH TEXT mkdir PATH rm PATH stat PATH\nmem uptime disk sync run PATH cp SOURCE DEST\nAttach an ATA data disk to keep file changes across reboot.\n");
        else if(equal(cmd,"clear"))console_clear();
        else if(equal(cmd,"pwd")){print(cwd);print("\n");}
        else if(equal(cmd,"mem")){print("Free physical pages: ");decimal(pages_free());print("\n");}
        else if(equal(cmd,"uptime")){print("Uptime (approx seconds): ");decimal(ticks/100);print("\n");}
        else if(equal(cmd,"disk")){if(!storage_ready())print("Disk: unavailable (RAM only)\n");else{print("Disk: ATA PIO, ");decimal(storage_sectors());print(" sectors, saved generation ");decimal(storage_generation());print("\n");}}
        else {int result=storage_save();if(result<0){print("error: sync failed (code ");decimal((uint64_t)(-result));print(")\n");}else{print("Saved generation ");decimal(storage_generation());print("\n");}}return;
    }
    bool cd=equal(cmd,"cd"),ls=equal(cmd,"ls");
    if(!(cd||ls||equal(cmd,"cat")||equal(cmd,"touch")||equal(cmd,"mkdir")||equal(cmd,"rm")||equal(cmd,"stat"))){print("error: unknown command: ");print(cmd);print("\n");return;}
    if(!arg&&!cd&&!ls){print("error: path required\n");return;}
    char path[FS_PATH];if(!path_for(arg?arg:(cd?"/":"."),path))return;
    struct fs_entry info;int result;
    if(cd){result=fs_stat(path,&info);if(!result&&info.type!=FS_DIR)result=-20;if(result<0)error(result);else canonical(path,cwd);}
    else if(ls){
        for(unsigned i=0;;++i){result=fs_list(path,i,&info);if(result<=0){if(result<0)error(result);break;}print(info.name);if(info.type==FS_DIR)putchar('/');print("\n");}
    }else if(equal(cmd,"mkdir")){result=fs_mkdir(path);if(result<0)error(result);else persist();}
    else if(equal(cmd,"rm")){
        result=fs_stat(path,&info);if(result<0){error(result);return;}
        char normal[FS_PATH];canonical(path,normal);size_t n=length(normal);
        bool ancestor=true;for(size_t i=0;i<n;++i)if(cwd[i]!=normal[i]){ancestor=false;break;}
        if(ancestor&&(n==1||cwd[n]==0||cwd[n]=='/')){error(-16);return;}
        result=fs_unlink(path);if(result<0)error(result);else persist();
    }else if(equal(cmd,"stat")){
        result=fs_stat(path,&info);if(result<0)error(result);else{print(info.type==FS_DIR?"directory ":"file ");decimal(info.size);print(" bytes\n");}
    }else{
        bool cat=equal(cmd,"cat");int fd=fs_open(files,path,cat?FS_READ:FS_WRITE|FS_CREATE);
        if(fd<0){error(fd);return;}
        if(cat){char data[128];int64_t count;bool any=false;char last=0;
            while((count=fs_read(files,fd,data,sizeof(data)))>0)for(int64_t i=0;i<count;++i){last=data[i];putchar(last=='\n'||(last>=32&&last<=126)?last:'.');any=true;}
            if(count<0)error((int)count);else if(any&&last!='\n')putchar('\n');
        }fs_close(files,fd);if(!cat)persist();
    }
}
_Noreturn void shell_run(void){
    char line[LINE_SIZE];size_t used=0;bool overflow=false;
    print("MyOS Shell ready. Type help.\nmyos> ");
    for(;;){
        uint64_t flags=irq_save();int c=keyboard_get();
        if(c<0){__asm__ volatile("sti; hlt":::"memory");continue;}irq_restore(flags);
        if(c=='\n'){
            putchar('\n');line[used]=0;
            if(overflow)print("error: line too long; command discarded\n");else execute(line);
            used=0;overflow=false;print("myos> ");
        }else if(c=='\b'){if(!overflow&&used){--used;putchar('\b');}}
        else if(c>=32&&c<=126&&!overflow){if(used==LINE_SIZE-1)overflow=true;else{line[used++]=(char)c;putchar((char)c);}}
    }
}
