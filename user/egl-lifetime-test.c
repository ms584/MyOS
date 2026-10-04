/* White-box native test support. Linked only when the fixture references it;
 * no production platform binding and no replacement renderer behavior. */
#include "eglcontext.h"
#include "eglsurface.h"
#include "eglsync.h"
int MyOSEglLifetimeTest(EGLDisplay display,EGLConfig config);
int MyOSEglSyncLifetimeTest(EGLDisplay display,EGLSync sync);
static void (*release_surface)(_EGLResource *),(*release_context)(_EGLResource *);
static unsigned surfaces,contexts;
static void (*release_sync)(_EGLResource *);
static unsigned syncs;
static void count_sync(_EGLResource *resource)
{release_sync(resource);++syncs;}
int MyOSEglSyncLifetimeTest(EGLDisplay display,EGLSync handle)
{
    _EGLSync *sync=(_EGLSync *)handle;
    release_sync=sync->Resource.OnFinalRelease;
    if(!release_sync)return 1;
    sync->Resource.OnFinalRelease=count_sync;
    unsigned before=syncs;_eglGetSync(sync);
    if(!eglDestroySync(display,handle)||syncs!=before)return 2;
    if(!_eglPutSync(sync)||syncs!=before+1)return 3;
    return 0;
}
static void count_surface(_EGLResource *resource)
{release_surface(resource);++surfaces;}
static void count_context(_EGLResource *resource)
{release_context(resource);++contexts;}
int MyOSEglLifetimeTest(EGLDisplay display,EGLConfig config)
{
    const EGLint attributes[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
    EGLSurface handle=eglCreatePbufferSurface(display,config,attributes);
    if(handle==EGL_NO_SURFACE)return 1;
    _EGLSurface *surface=(_EGLSurface *)handle;
    release_surface=surface->Resource.OnFinalRelease;
    if(!release_surface)return 2;
    surface->Resource.OnFinalRelease=count_surface;
    unsigned before=surfaces;
    _eglGetSurface(surface); /* exactly the pin acquired by egl_relax */
    if(!eglDestroySurface(display,handle)||surfaces!=before)return 3;
    if(!_eglPutSurface(surface)||surfaces!=before+1)return 4;
    const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext context_handle=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
    if(context_handle==EGL_NO_CONTEXT)return 5;
    _EGLContext *context=(_EGLContext *)context_handle;
    release_context=context->Resource.OnFinalRelease;
    if(!release_context)return 6;
    context->Resource.OnFinalRelease=count_context;
    before=contexts;_eglGetContext(context);
    if(!eglDestroyContext(display,context_handle)||contexts!=before)return 7;
    if(!_eglPutContext(context)||contexts!=before+1)return 8;
    return 0;
}
