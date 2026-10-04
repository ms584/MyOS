/* MyOS software EGL driver for genuine Mesa core/OSMesa GLES.
 * Initial pbuffer/context slice only. EGL core derives version1.4: no fake
 * image/sync/colorspace extensions or EGL1.5/Cobalt acceptance claim. */
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <GL/glext.h>
#include <stdlib.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "eglsurface.h"
struct myos_display { unsigned references; };
struct myos_context { _EGLContext base; OSMesaContext mesa; };
struct myos_surface { _EGLSurface base; void *pixels; };
static EGLBoolean fail(EGLint code,const char *where){return _eglError(code,where);}
static void display_get(_EGLDisplay *display)
{__atomic_add_fetch(&((struct myos_display *)display->DriverData)->references,1,__ATOMIC_RELAXED);}
static void display_put(_EGLDisplay *display)
{
    struct myos_display *state=display->DriverData;
    if(!__atomic_sub_fetch(&state->references,1,__ATOMIC_ACQ_REL)){
        _eglCleanupDisplay(display);display->DriverData=0;free(state);
    }
}
static EGLBoolean initialize(_EGLDisplay *display)
{
    if(display->Platform!=_EGL_PLATFORM_SURFACELESS)
        return fail(EGL_NOT_INITIALIZED,"MyOS EGL platform");
    if(display->DriverData){display_get(display);return EGL_TRUE;}
    struct myos_display *state=calloc(1,sizeof(*state));
    _EGLConfig *config=calloc(1,sizeof(*config));
    if(!state||!config){free(state);free(config);return fail(EGL_BAD_ALLOC,"eglInitialize");}
    _eglInitConfig(config,display,1);
    config->BufferSize=32;config->RedSize=config->GreenSize=config->BlueSize=config->AlphaSize=8;
    config->RenderableType=EGL_OPENGL_ES2_BIT;config->Conformant=0; /* not certified */
    config->SurfaceType=EGL_PBUFFER_BIT;
    config->MaxPbufferWidth=config->MaxPbufferHeight=1024;
    config->MaxPbufferPixels=1024*1024;
    config->MinSwapInterval=config->MaxSwapInterval=0;
    if(!_eglValidateConfig(config,EGL_FALSE)){
        free(config);free(state);return fail(EGL_NOT_INITIALIZED,"MyOS EGL config");
    }
    if(!_eglLinkConfig(config)){
        free(config);free(state);return fail(EGL_BAD_ALLOC,"MyOS EGL config link");
    }
    display->ClientAPIs=EGL_OPENGL_ES2_BIT;
    state->references=1;display->DriverData=state;return EGL_TRUE;
}
static EGLBoolean terminate(_EGLDisplay *display)
{_eglReleaseDisplayResources(display);display_put(display);return EGL_TRUE;}
static _EGLContext *create_context(_EGLDisplay *display,_EGLConfig *config,
                                 _EGLContext *share,const EGLint *attributes)
{
    struct myos_context *context=calloc(1,sizeof(*context));
    if(!context){fail(EGL_BAD_ALLOC,"eglCreateContext");return 0;}
    if(!_eglInitContext(&context->base,display,config,share,attributes)){free(context);return 0;}
    if(context->base.ClientAPI!=EGL_OPENGL_ES_API||context->base.ClientMajorVersion!=2||
       context->base.ClientMinorVersion||context->base.Flags||context->base.NoError||
       context->base.ResetNotificationStrategy!=EGL_NO_RESET_NOTIFICATION){
        free(context);fail(EGL_BAD_MATCH,"MyOS EGL context attributes");return 0;
    }
    const int options[]={OSMESA_FORMAT,OSMESA_RGBA,OSMESA_DEPTH_BITS,0,
        OSMESA_STENCIL_BITS,0,OSMESA_ACCUM_BITS,0,OSMESA_PROFILE,OSMESA_MYOS_GLES_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION,2,OSMESA_CONTEXT_MINOR_VERSION,0,0};
    context->mesa=OSMesaCreateContextAttribs(options,share?((struct myos_context *)share)->mesa:0);
    if(!context->mesa){free(context);fail(EGL_BAD_ALLOC,"MyOS GLES context");return 0;}
    display_get(display);return &context->base;
}
static EGLBoolean destroy_context(_EGLDisplay *display,_EGLContext *base)
{
    if(base&&_eglPutContext(base)){
        struct myos_context *context=(void *)base;
        OSMesaDestroyContext(context->mesa);free(context);display_put(display);
    }
    return EGL_TRUE;
}
static _EGLSurface *create_pbuffer(_EGLDisplay *display,_EGLConfig *config,const EGLint *attributes)
{
    struct myos_surface *surface=calloc(1,sizeof(*surface));
    if(!surface){fail(EGL_BAD_ALLOC,"eglCreatePbufferSurface");return 0;}
    if(!_eglInitSurface(&surface->base,display,EGL_PBUFFER_BIT,config,attributes,0)){
        free(surface);return 0;
    }
    int width=surface->base.Width,height=surface->base.Height;
    /* No render-to-texture storage is implemented or advertised. Mesa's
     * generic surface parser checks attribute pairs, not driver capability. */
    if(surface->base.TextureFormat!=EGL_NO_TEXTURE||surface->base.TextureTarget!=EGL_NO_TEXTURE){
        free(surface);fail(EGL_BAD_MATCH,"MyOS EGL texture pbuffer");return 0;
    }
    if(surface->base.LargestPbuffer){
        if(width>config->MaxPbufferWidth)width=config->MaxPbufferWidth;
        if(height>config->MaxPbufferHeight)height=config->MaxPbufferHeight;
        surface->base.Width=width;surface->base.Height=height;
    }
    if(width<0||height<0||width>config->MaxPbufferWidth||height>config->MaxPbufferHeight){
        free(surface);fail(EGL_BAD_ALLOC,"MyOS EGL pbuffer dimensions");return 0;
    }
    /* Zero-sized pbuffers are valid EGL objects; binding needs a backing
     * pixel because OSMesa rejects zero-sized buffers. Query remains exact. */
    surface->pixels=calloc(width? (size_t)width:1,(height?(size_t)height:1)*4);
    if(!surface->pixels){free(surface);fail(EGL_BAD_ALLOC,"MyOS EGL pbuffer storage");return 0;}
    return &surface->base;
}
static EGLBoolean destroy_surface(_EGLDisplay *display,_EGLSurface *base)
{
    (void)display;
    if(base&&_eglPutSurface(base)){
        struct myos_surface *surface=(void *)base;free(surface->pixels);free(surface);
    }
    return EGL_TRUE;
}
static EGLBoolean make_current(_EGLDisplay *display,_EGLSurface *draw,_EGLSurface *read,_EGLContext *base)
{
    if(base&&(!draw||draw!=read))return fail(EGL_BAD_MATCH,"MyOS EGL draw/read");
    _EGLContext *old;_EGLSurface *old_draw,*old_read;
    if(!_eglBindContext(base,draw,read,&old,&old_draw,&old_read))return EGL_FALSE;
    if(old)glFlush();
    struct myos_surface *surface=(void *)draw;
    EGLBoolean bound=OSMesaMakeCurrent(base?((struct myos_context *)base)->mesa:0,
        surface?surface->pixels:0,GL_UNSIGNED_BYTE,
        surface?(surface->base.Width?surface->base.Width:1):0,
        surface?(surface->base.Height?surface->base.Height:1):0);
    if(!bound){
        _EGLContext *new_context;_EGLSurface *new_draw,*new_read;
        _eglBindContext(old,old_draw,old_read,&new_context,&new_draw,&new_read);
        destroy_surface(display,new_draw);destroy_surface(display,new_read);
        destroy_context(display,new_context);
    }
    _EGLDisplay *old_display=old?old->Resource.Display:display;
    destroy_surface(old_display,old_draw);destroy_surface(old_display,old_read);
    destroy_context(old_display,old);
    return bound?EGL_TRUE:fail(EGL_BAD_MATCH,"MyOS GLES binding");
}
static EGLBoolean swap_buffers(_EGLDisplay *display,_EGLSurface *surface)
{(void)display;(void)surface;glFlush();return EGL_TRUE;} /* pbuffer has no native swap */
static EGLBoolean wait_client(_EGLDisplay *display,_EGLContext *context)
{(void)display;(void)context;glFinish();return EGL_TRUE;}
static EGLBoolean wait_native(EGLint engine)
{return engine==EGL_CORE_NATIVE_ENGINE?EGL_TRUE:fail(EGL_BAD_PARAMETER,"eglWaitNative");}
static EGLBoolean swap_interval(_EGLDisplay *display,_EGLSurface *surface,EGLint interval)
{(void)display;(void)surface;(void)interval;return EGL_TRUE;} /* core clamps to supported0 */
static EGLBoolean copy_buffers(_EGLDisplay *display,_EGLSurface *surface,void *pixmap)
{(void)display;(void)surface;(void)pixmap;return fail(EGL_BAD_NATIVE_PIXMAP,"MyOS EGL has no native pixmaps");}
const _EGLDriver _eglDriver={.Initialize=initialize,.Terminate=terminate,
    .CreateContext=create_context,.DestroyContext=destroy_context,.MakeCurrent=make_current,
    .CreatePbufferSurface=create_pbuffer,.DestroySurface=destroy_surface,.QuerySurface=_eglQuerySurface,
    /* These core helpers always reject binding our EGL_NO_TEXTURE pbuffers,
     * and release of an unbound image is the defined no-op. Never NULL-call. */
    .BindTexImage=_eglBindTexImage,.ReleaseTexImage=_eglReleaseTexImage,.CopyBuffers=copy_buffers,
    .SwapBuffers=swap_buffers,.SwapInterval=swap_interval,.WaitClient=wait_client,.WaitNative=wait_native};
