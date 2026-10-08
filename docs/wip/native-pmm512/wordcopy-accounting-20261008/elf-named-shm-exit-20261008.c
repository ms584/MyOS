#include <fcntl.h>
#include <unistd.h>
extern void SbLogRaw(const char *);
int _start(void){
 int fd=openat(AT_FDCWD,"/dev/shm/creator-exit-shm",O_CREAT|O_EXCL|O_RDWR,0600);
 if(fd<0)return 90;
 if(ftruncate(fd,4096)||close(fd))return 91;
 SbLogRaw("Named SHM intentionally retained; no mappings and fd closed before exit\n");
 return 88;
}
