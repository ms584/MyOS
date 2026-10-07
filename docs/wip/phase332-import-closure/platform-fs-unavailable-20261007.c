/* Import closure only, NOT implemented POSIX services. Pinned musl LP64 ABI.
 * Return ENOSYS rather than success, invented permission/timestamp state,
 * fake symlinks or fake inotify descriptors. Replace each function with its
 * actual kernel service and behavior tests before claiming semantic coverage.
 */
#include <stdint.h>
#include <stddef.h>
struct fs_timespec {int64_t seconds,nanoseconds;};
_Static_assert(sizeof(struct fs_timespec)==16,"pinned timespec ABI");
extern int *__errno_location(void);
static int unavailable(void){*__errno_location()=38;return -1;}
int chmod(const char *path,uint32_t mode){(void)path;(void)mode;return unavailable();}
int fchmod(int fd,uint32_t mode){(void)fd;(void)mode;return unavailable();}
int fchown(int fd,uint32_t owner,uint32_t group)
{(void)fd;(void)owner;(void)group;return unavailable();}
int utimensat(int directory,const char *path,const struct fs_timespec times[2],int flags)
{(void)directory;(void)path;(void)times;(void)flags;return unavailable();}
int symlink(const char *target,const char *path){(void)target;(void)path;return unavailable();}
int64_t readlink(const char *path,char *buffer,size_t capacity)
{(void)path;(void)buffer;(void)capacity;return unavailable();}
int inotify_init1(int flags){(void)flags;return unavailable();}
int inotify_add_watch(int fd,const char *path,uint32_t mask)
{(void)fd;(void)path;(void)mask;return unavailable();}
int inotify_rm_watch(int fd,int watch){(void)fd;(void)watch;return unavailable();}
