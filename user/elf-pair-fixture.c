/* Pair lifecycle adversaries and genuine provider dispatch, NOT Cobalt. */
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static void sleep_tick(void){struct timespec delay={0,10000000};clock_nanosleep(1,0,&delay,0);}
#if defined(ACTUAL_RENDERER)
#include <EGL/egl.h>
#include <GL/gl.h>
#include "starboard/egl.h"
#include "starboard/gles.h"
extern const SbEglInterface *SbGetEglInterface(void);
extern const SbGlesInterface *SbGetGlesInterface(void);
#else
extern const void *SbGetEglInterface(void);
extern const void *SbGetGlesInterface(void);
#endif
static void say(const char *s){size_t n=0;while(s[n])++n;write(1,s,n);}
#define CHECK(x) do{if(!(x)){say("pair FAIL: " #x "\n");__builtin_trap();}}while(0)
int fflush(FILE *stream){(void)stream;return 0;}
#if defined(PROVIDER)
static const uint64_t token=7;
const void *SbGetEglInterface(void){return &token;}
const void *SbGetGlesInterface(void){return &token;}
#define OWNER "provider"
#else
#define OWNER "core"
#endif
extern int __cxa_atexit(void (*)(void *),void *,void *);
extern void *__dso_handle;
static void registered_callback(void *argument);
__attribute__((constructor)) static void initialize(void)
{
    say("pair " OWNER " init\n");
    CHECK(!__cxa_atexit(registered_callback,0,&__dso_handle));
#if defined(CTOR_EXIT)
    exit(77);
#endif
}
__attribute__((destructor)) static void finalize(void)
{
    say("pair " OWNER " fini\n");
#if defined(FAULT_FINI)
    __builtin_trap();
#elif defined(LOOP_FINI)
    say("pair looping finalizer ready\n");for(;;)sleep_tick();
#endif
}
static void registered_callback(void *argument)
{
    (void)argument;say("pair " OWNER " cxa\n");
#if defined(CONCURRENT_EXIT)
    /* Ensure the competing worker can run while the cleanup owner is alive. */
    for(unsigned i=0;i<8;++i)sleep_tick();
#endif
#if defined(RECURSIVE_EXIT)
    exit(78);
#endif
}
void SbEventHandle(const void *event){(void)event;}
const char *GetEvergreenSabiString(void){return "deliberately incompatible paired SABI";}
#if defined(ACTUAL_RENDERER)
static void *render(void *argument)
{
    (void)argument;
    const SbEglInterface *e=SbGetEglInterface();const SbGlesInterface *g=SbGetGlesInterface();
    CHECK(e&&g);
    CHECK(e->eglGetCurrentContext()==EGL_NO_CONTEXT&&e->eglGetCurrentDisplay()==EGL_NO_DISPLAY);
    EGLDisplay d=e->eglGetDisplay(0);CHECK(d!=EGL_NO_DISPLAY);
    EGLint major=0,minor=0;CHECK(e->eglInitialize(d,&major,&minor)&&major==1&&minor==5);
    CHECK(e->eglBindAPI(EGL_OPENGL_ES_API));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_NONE};
    EGLConfig config=0;EGLint count=0;CHECK(e->eglChooseConfig(d,attributes,&config,1,&count)&&count==1);
    EGLint sizes[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};
    EGLSurface surface=e->eglCreatePbufferSurface(d,config,sizes);CHECK(surface!=EGL_NO_SURFACE);
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext context=e->eglCreateContext(d,config,EGL_NO_CONTEXT,version);CHECK(context!=EGL_NO_CONTEXT);
    CHECK(e->eglMakeCurrent(d,surface,surface,context));
    CHECK(e->eglGetCurrentContext()==context&&e->eglGetCurrentDisplay()==d);
    g->glClearColor(0,1,1,1);g->glClear(GL_COLOR_BUFFER_BIT);
    unsigned char pixel[4]={0};g->glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    CHECK(pixel[0]==0&&pixel[1]==255&&pixel[2]==255&&pixel[3]==255);
    CHECK(g->glGetError()==GL_NO_ERROR);
    CHECK(e->eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    CHECK(e->eglGetCurrentContext()==EGL_NO_CONTEXT&&e->eglGetCurrentDisplay()==EGL_NO_DISPLAY);
    CHECK(e->eglDestroyContext(d,context)&&e->eglDestroySurface(d,surface));
    CHECK(e->eglReleaseThread()&&e->eglTerminate(d));
    say("pair genuine EGL/GLES pixel/TLS PASS\n");return (void *)(uintptr_t)91;
}
#endif
static volatile unsigned exit_ready;
static void *worker(void *argument)
{
    (void)argument;
#if defined(WORKER_EXIT) || defined(CONCURRENT_EXIT)
#if defined(CONCURRENT_EXIT)
    while(!__atomic_load_n(&exit_ready,__ATOMIC_ACQUIRE))sleep_tick();
#endif
    say("pair worker exit\n");exit(77);
#else
    for(;;)sleep_tick();
#endif
}
int _start(void)
{
    CHECK(SbGetEglInterface()&&SbGetGlesInterface());say("pair core entry\n");
#if defined(FAULT_ENTRY)
    __builtin_trap();
#endif
#if defined(CONTROL_EXEC)
    ((void (*)(void))(uintptr_t)0x8010011000ull)();
#endif
#if defined(ACTUAL_RENDERER)
    for(unsigned i=0;i<2;++i){pthread_t t;void *result=0;
        CHECK(!pthread_create(&t,0,render,0));
        CHECK(!pthread_join(t,&result)&&result==(void *)(uintptr_t)91);}
#endif
#if defined(CONCURRENT_EXIT)
    pthread_t tasks[8];for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&tasks[i],0,worker,0));
    __atomic_store_n(&exit_ready,1,__ATOMIC_RELEASE);
    for(;;)sleep_tick();
#elif defined(WORKER_EXIT) || defined(LOOP)
    pthread_t t;CHECK(!pthread_create(&t,0,worker,0));
    say("pair waiting ready\n");for(;;)sleep_tick();
#elif defined(RAW_EXIT)
    _Exit(77);
#else
    return 77;
#endif
}
