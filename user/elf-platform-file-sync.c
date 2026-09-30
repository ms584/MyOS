#include <stdint.h>
#include <stddef.h>
extern int *__errno_location(void);
extern int openat(int,const char *,int,...),close(int),fsync(int),fdatasync(int);
extern int64_t write(int,const void *,size_t),read(int,void *,size_t),lseek(int,int64_t,int);
static void print(const char *text)
{size_t length=0;while(text[length])++length;(void)write(1,text,length);}
uint64_t _start(void)
{
    int fd=openat(-100,"/home/abi-persist",2,0),created=0;
    if(fd<0){
        if(*__errno_location()!=2)return 1;
        fd=openat(-100,"/home/abi-persist",2|0100|0200,0600);created=1;
    }
    if(fd<64)return 2;
    unsigned char block[1024];
    if(created){
        for(unsigned i=0;i<sizeof(block);++i)block[i]=(unsigned char)(i%251);
        for(unsigned i=0;i<8;++i)if(write(fd,block,sizeof(block))!=sizeof(block))return 3;
        if(fsync(fd)||fdatasync(fd))return 4;
    }
    if(lseek(fd,0,2)!=8192||lseek(fd,0,0))return 5;
    for(unsigned i=0;i<8;++i){
        if(read(fd,block,sizeof(block))!=sizeof(block))return 6;
        for(unsigned j=0;j<sizeof(block);++j)if(block[j]!=(unsigned char)(j%251))return 7;
    }
    if(close(fd))return 8;
    print(created?"ELF file fsync committed binary data PASS\n":"ELF file fsync reboot persistence PASS\n");
    return 39;
}
