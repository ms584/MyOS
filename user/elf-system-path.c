#include "starboard/system.h"
#include "event.h"
#include "starboard/log.h"
#include "starboard-sabi.h"
#include "system-call.h"
static int equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
#define CHECK(x) do{if(!(x)){SbLogRaw("system launcher failed: " #x "\n");__builtin_trap();}}while(0)
const char *GetEvergreenSabiString(void){return MYOS_STARBOARD_SABI;}
void SbEventHandle(const SbEvent *event)
{
 if(event->type!=kSbEventTypeStart)return;
 SbEventStartData *start=event->data;CHECK(start&&start->argument_count==1);
 const char *name=start->argument_values[0];char expected[80]="/assets/";
 unsigned n=0;while(name[n]){CHECK(n<64);expected[n+8]=name[n];++n;}expected[n+8]=0;
 char path[80];CHECK(SbSystemGetPath(kSbSystemPathExecutableFile,path,sizeof(path))&&equal(path,expected));
 /* The startup argv page is read-only; rejected requests must not alter it. */
 CHECK(!SbSystemGetPath(kSbSystemPathContentDirectory,(char *)name,80));
 CHECK(equal(name,"pathlaunch"));
 SbLogRaw("system owned executable path PASS\n");SbSystemRequestStop(87);
}
