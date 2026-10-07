#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef ABSENT
#define ABSENT 0
#endif
extern bool SbSystemGetPath(int,char *,int);
extern void SbLogRaw(const char *);
static int equal(const char *a,const char *b)
{while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void fill(void *raw,size_t n)
{unsigned char *p=raw;for(size_t i=0;i<n;++i)p[i]=0xa5;}
#define CHECK(x) do{if(!(x)){SbLogRaw("Native fonts FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void)
{
 struct {unsigned char before[16];char path[128];unsigned char after[16];} guard;
 fill(&guard,sizeof(guard));
 if(ABSENT){
  CHECK(!SbSystemGetPath(3,guard.path,128)&&errno==95);
  CHECK(!SbSystemGetPath(4,guard.path,128)&&errno==95);
  for(unsigned i=0;i<sizeof(guard);++i)CHECK(((unsigned char *)&guard)[i]==0xa5);
  SbLogRaw("native font directories honestly unavailable PASS\n");return 90;
 }
 CHECK(SbSystemGetPath(3,guard.path,128)&&equal(guard.path,"/assets/systemfonts"));
 CHECK(SbSystemGetPath(4,guard.path,128)&&equal(guard.path,"/assets/systemfonts"));
 for(unsigned i=0;i<16;++i)CHECK(guard.before[i]==0xa5&&guard.after[i]==0xa5);
 fill(guard.path,128);CHECK(!SbSystemGetPath(3,guard.path,19)&&errno==22);
 for(unsigned i=0;i<128;++i)CHECK((unsigned char)guard.path[i]==0xa5);
 CHECK(!SbSystemGetPath(4,(char *)1,128)&&errno==14);
 CHECK(!SbSystemGetPath(-1,guard.path,128)&&errno==22);
 const char *fonts[]={"/assets/systemfonts/DejaVuSans.ttf","/assets/systemfonts/DejaVuSansMono.ttf","/assets/systemfonts/DejaVuSerif.ttf"};
 const int64_t sizes[]={759720,343140,380660};
 for(unsigned i=0;i<3;++i){
  int fd=openat(AT_FDCWD,fonts[i],O_RDONLY);CHECK(fd>=0);struct stat st;unsigned char signature[4];
  CHECK(!fstat(fd,&st)&&st.st_size==sizes[i]&&read(fd,signature,4)==4);
  CHECK(!signature[0]&&signature[1]==1&&!signature[2]&&!signature[3]);
  CHECK(!close(fd)&&openat(AT_FDCWD,fonts[i],O_RDWR)==-1&&errno==30);
 }
 int fd=openat(AT_FDCWD,"/assets/systemfonts/fonts.xml",O_RDONLY);CHECK(fd>=0);char header[5];
 CHECK(read(fd,header,5)==5&&header[0]=='<'&&header[1]=='?'&&!close(fd));
 fd=openat(AT_FDCWD,"/assets/systemfonts/LICENSE.txt",O_RDONLY);CHECK(fd>=0&&read(fd,header,5)==5&&!close(fd));
 SbLogRaw("native platform font directories/sfnt reads/config/license/readonly/guards PASS\n");return 90;
}
