
#include <stdint.h>
#include <stdbool.h>
struct size {int width,height;float ratio;};
struct options {struct size size;bool windowed;const char *name;};
extern void SbWindowSetDefaultOptions(struct options *);
extern void *SbWindowCreate(const struct options *);
extern bool SbWindowDestroy(void *);
extern bool SbWindowGetSize(void *,struct size *);
extern void SbLogRaw(const char *);
#define CHECK(x) do{if(!(x)){SbLogRaw("window reproduction FAIL: " #x "\n");return __LINE__;}}while(0)
int _start(void){
 struct options o;SbWindowSetDefaultOptions(&o);
 CHECK(o.size.width==800&&o.size.height==600);
 void *main=SbWindowCreate(&o);CHECK(main);
 o.size.width=200;o.size.height=30;
 CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO small native window rejected while primary live\n");
 SbWindowSetDefaultOptions(&o);CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO second full-size native window EBUSY\n");
 struct size dimensions;CHECK(SbWindowGetSize(main,&dimensions)&&dimensions.width==800&&dimensions.height==600);
 CHECK(SbWindowDestroy(main));
 o.size.width=200;o.size.height=30;CHECK(!SbWindowCreate(&o));
 SbLogRaw("REPRO small native window also rejected without primary\n");
 SbWindowSetDefaultOptions(&o);main=SbWindowCreate(&o);CHECK(main&&SbWindowDestroy(main));
 SbLogRaw("native window rejection reproduced, primary lifetime intact\n");return 90;
}
