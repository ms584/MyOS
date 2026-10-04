/* MyOS software EGL driver for genuine Mesa core/OSMesa GLES.
 * Native window/pbuffer, real image storage and Gallium fence adapters.
 * EGL core still derives version1.4; no EGL1.5/Cobalt acceptance claim. */
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <GL/glext.h>
#include <stdlib.h>
#include <stdint.h>
#include "native-ui.h"
extern int MyOSUiQuery(void *,struct native_ui_info *);
extern int MyOSUiPresent(void *,const struct native_ui_present *);
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "eglsurface.h"
#include "eglsync.h"
#include "eglimage.h"
#include "frontend/api.h"
extern int MyOSMesaInitializeImages(void);
extern void *MyOSMesaCreateImage(OSMesaContext,unsigned,unsigned,unsigned,int,int *);
extern void MyOSMesaDestroyImage(void *);
extern void MyOSMesaTranslateImage(void *,struct st_egl_image *);
bool MyOSEglGetImage(void *,struct st_egl_image *);
extern void *MyOSMesaCreateFence(void);
extern int MyOSMesaWaitFence(void *,uint64_t);
extern int MyOSMesaServerWaitFence(void *);
extern void MyOSMesaDestroyFence(void *);
struct myos_display { unsigned references; };
struct myos_context { _EGLContext base; OSMesaContext mesa; };
struct myos_surface { _EGLSurface base; void *pixels; };
struct myos_sync { _EGLSync base; void *fence; };
struct myos_image { _EGLImage base; void *mesa; };
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
    if(display->Platform!=_EGL_PLATFORM_SURFACELESS&&display->Platform!=_EGL_PLATFORM_MYOS)
        return fail(EGL_NOT_INITIALIZED,"MyOS EGL platform");
    if(display->Platform==_EGL_PLATFORM_MYOS&&display->PlatformDisplay)
        return fail(EGL_NOT_INITIALIZED,"MyOS EGL requires default display");
    /* Install frontend image callbacks before Mesa creates a client context. */
    if(!MyOSMesaInitializeImages())return fail(EGL_NOT_INITIALIZED,"MyOS Mesa image manager");
    if(display->DriverData){display_get(display);return EGL_TRUE;}
    struct myos_display *state=calloc(1,sizeof(*state));
    _EGLConfig *config=calloc(1,sizeof(*config));
    if(!state||!config){free(state);free(config);return fail(EGL_BAD_ALLOC,"eglInitialize");}
    _eglInitConfig(config,display,1);
    config->BufferSize=32;config->RedSize=config->GreenSize=config->BlueSize=config->AlphaSize=8;
    config->RenderableType=EGL_OPENGL_ES2_BIT;config->Conformant=0; /* not certified */
    config->SurfaceType=EGL_PBUFFER_BIT;
    if(display->Platform==_EGL_PLATFORM_MYOS)config->SurfaceType|=EGL_WINDOW_BIT;
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
    display->Extensions.KHR_fence_sync=EGL_TRUE;
    display->Extensions.KHR_wait_sync=EGL_TRUE;
    display->Extensions.KHR_image_base=EGL_TRUE;
    display->Extensions.KHR_gl_texture_2D_image=EGL_TRUE;
    display->Extensions.KHR_gl_texture_cubemap_image=EGL_TRUE;
    display->Extensions.KHR_gl_texture_3D_image=EGL_TRUE;
    display->Extensions.KHR_gl_renderbuffer_image=EGL_TRUE;
    state->references=1;display->DriverData=state;return EGL_TRUE;
}
static EGLBoolean terminate(_EGLDisplay *display)
{_eglReleaseDisplayResources(display);display_put(display);return EGL_TRUE;}
static void free_context(_EGLResource *resource)
{
    _EGLDisplay *display=resource->Display;
    struct myos_context *context=(void *)resource;
    OSMesaDestroyContext(context->mesa);free(context);display_put(display);
}
static void free_surface(_EGLResource *resource)
{struct myos_surface *surface=(void *)resource;free(surface->pixels);free(surface);}
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
    context->base.Resource.OnFinalRelease=free_context;
    display_get(display);return &context->base;
}
static EGLBoolean destroy_context(_EGLDisplay *display,_EGLContext *base)
{
    (void)display;
    if(base)_eglPutContext(base);
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
    surface->base.Resource.OnFinalRelease=free_surface;
    return &surface->base;
}
static EGLBoolean destroy_surface(_EGLDisplay *display,_EGLSurface *base)
{
    (void)display;
    if(base)_eglPutSurface(base);
    return EGL_TRUE;
}
static _EGLSurface *create_window(_EGLDisplay *display,_EGLConfig *config,
                                void *window,const EGLint *attributes)
{
    struct native_ui_info info;
    if(display->Platform!=_EGL_PLATFORM_MYOS||MyOSUiQuery(window,&info)||
       !window||info.version!=NATIVE_UI_ABI||!info.width||!info.height){
        fail(EGL_BAD_NATIVE_WINDOW,"MyOS EGL window");return 0;
    }
    struct myos_surface *surface=calloc(1,sizeof(*surface));
    if(!surface){fail(EGL_BAD_ALLOC,"MyOS EGL window");return 0;}
    if(!_eglInitSurface(&surface->base,display,EGL_WINDOW_BIT,config,attributes,window)){
        free(surface);return 0;
    }
    /* MyOS's boot framebuffer is fixed; checked products avoid size overflow. */
    if(info.width>INT32_MAX/4||info.height>INT32_MAX||
       (size_t)info.width>SIZE_MAX/4/info.height){
        free(surface);fail(EGL_BAD_ALLOC,"MyOS EGL window size");return 0;
    }
    surface->base.Width=(EGLint)info.width;surface->base.Height=(EGLint)info.height;
    surface->pixels=calloc((size_t)info.width*info.height,4);
    if(!surface->pixels){free(surface);fail(EGL_BAD_ALLOC,"MyOS EGL window pixels");return 0;}
    surface->base.Resource.OnFinalRelease=free_surface;
    return &surface->base;
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
    }else if(base)OSMesaPixelStore(OSMESA_Y_UP,draw->Type==EGL_WINDOW_BIT?0:1);
    _EGLDisplay *old_display=old?old->Resource.Display:display;
    destroy_surface(old_display,old_draw);destroy_surface(old_display,old_read);
    destroy_context(old_display,old);
    return bound?EGL_TRUE:fail(EGL_BAD_MATCH,"MyOS GLES binding");
}
static EGLBoolean swap_buffers(_EGLDisplay *display,_EGLSurface *surface)
{
    (void)display;
    if(surface->Type!=EGL_WINDOW_BIT){glFlush();return EGL_TRUE;}
    _EGLContext *current=_eglGetCurrentContext();
    if(!current||current->DrawSurface!=surface)return fail(EGL_BAD_SURFACE,"MyOS EGL swap ownership");
    struct native_ui_info info;
    if(MyOSUiQuery(surface->NativeSurface,&info)||info.width!=(uint32_t)surface->Width||
       info.height!=(uint32_t)surface->Height)return fail(EGL_BAD_NATIVE_WINDOW,"MyOS EGL stale window");
    /* Finish is required before the kernel reads CPU-owned softpipe storage. */
    glFinish();
    struct native_ui_present present={(uintptr_t)((struct myos_surface *)surface)->pixels,
        (uint32_t)surface->Width,(uint32_t)surface->Height,(uint32_t)surface->Width*4,
        NATIVE_UI_RGBA8,0,0};
    return MyOSUiPresent(surface->NativeSurface,&present)?
        fail(EGL_BAD_NATIVE_WINDOW,"MyOS EGL native present"):EGL_TRUE;
}
static EGLBoolean wait_client(_EGLDisplay *display,_EGLContext *context)
{(void)display;(void)context;glFinish();return EGL_TRUE;}
static EGLBoolean wait_native(EGLint engine)
{return engine==EGL_CORE_NATIVE_ENGINE?EGL_TRUE:fail(EGL_BAD_PARAMETER,"eglWaitNative");}
static EGLBoolean swap_interval(_EGLDisplay *display,_EGLSurface *surface,EGLint interval)
{(void)display;(void)surface;(void)interval;return EGL_TRUE;} /* core clamps to supported0 */
static EGLBoolean copy_buffers(_EGLDisplay *display,_EGLSurface *surface,void *pixmap)
{(void)display;(void)surface;(void)pixmap;return fail(EGL_BAD_NATIVE_PIXMAP,"MyOS EGL has no native pixmaps");}
static void free_sync(_EGLResource *resource)
{
    _EGLDisplay *display=resource->Display;struct myos_sync *sync=(void *)resource;
    MyOSMesaDestroyFence(sync->fence);free(sync);display_put(display);
}
static _EGLSync *create_sync(_EGLDisplay *display,EGLenum type,const EGLAttrib *attributes)
{
    if(type!=EGL_SYNC_FENCE_KHR){fail(EGL_BAD_ATTRIBUTE,"MyOS EGL sync type");return 0;}
    struct myos_sync *sync=calloc(1,sizeof(*sync));
    if(!sync){fail(EGL_BAD_ALLOC,"MyOS EGL fence");return 0;}
    if(!_eglInitSync(&sync->base,display,type,attributes)){free(sync);return 0;}
    sync->fence=MyOSMesaCreateFence();
    if(!sync->fence){free(sync);fail(EGL_BAD_ALLOC,"MyOS Mesa fence");return 0;}
    sync->base.Resource.OnFinalRelease=free_sync;display_get(display);return &sync->base;
}
static EGLBoolean destroy_sync(_EGLDisplay *display,_EGLSync *sync)
{(void)display;if(sync)_eglPutSync(sync);return EGL_TRUE;}
static EGLint client_wait_sync(_EGLDisplay *display,_EGLSync *base,EGLint flags,EGLTime timeout)
{
    (void)display;
    if(flags&EGL_SYNC_FLUSH_COMMANDS_BIT_KHR){
        _EGLContext *current=_eglGetCurrentContext();if(current)glFlush();
    }
    if(!MyOSMesaWaitFence(((struct myos_sync *)base)->fence,timeout))return EGL_TIMEOUT_EXPIRED_KHR;
    __atomic_store_n(&base->SyncStatus,EGL_SIGNALED_KHR,__ATOMIC_RELEASE);
    return EGL_CONDITION_SATISFIED_KHR;
}
static EGLint server_wait_sync(_EGLDisplay *display,_EGLSync *sync)
{(void)display;return MyOSMesaServerWaitFence(((struct myos_sync *)sync)->fence)?
    EGL_TRUE:fail(EGL_BAD_MATCH,"MyOS Mesa server fence wait");}
static void free_image(_EGLResource *resource)
{
    _EGLDisplay *display=resource->Display;struct myos_image *image=(void *)resource;
    MyOSMesaDestroyImage(image->mesa);free(image);display_put(display);
}
static _EGLImage *create_image(_EGLDisplay *display,_EGLContext *context,EGLenum target,
                             EGLClientBuffer buffer,const EGLint *attributes)
{
    if(!context){fail(EGL_BAD_CONTEXT,"MyOS EGL image context");return 0;}
    if((uintptr_t)buffer>UINT32_MAX){fail(EGL_BAD_PARAMETER,"MyOS EGL image name");return 0;}
    _EGLImageAttribs attrs;if(!_eglParseImageAttribList(&attrs,display,attributes))return 0;
    unsigned gl_target,layer=0;
    switch(target){
    case EGL_GL_TEXTURE_2D_KHR:gl_target=GL_TEXTURE_2D;break;
    case EGL_GL_TEXTURE_3D_KHR:
        if(attrs.GLTextureZOffset<0){fail(EGL_BAD_MATCH,"MyOS EGL image layer");return 0;}
        gl_target=GL_TEXTURE_3D;layer=(unsigned)attrs.GLTextureZOffset;break;
    case EGL_GL_RENDERBUFFER_KHR:gl_target=GL_RENDERBUFFER;break;
    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
        gl_target=GL_TEXTURE_CUBE_MAP;layer=target-EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR;break;
    default:fail(EGL_BAD_PARAMETER,"MyOS EGL image target");return 0;
    }
    struct myos_image *image=calloc(1,sizeof(*image));
    if(!image){fail(EGL_BAD_ALLOC,"MyOS EGL image");return 0;}
    _eglInitImage(&image->base,display);int error;
    image->mesa=MyOSMesaCreateImage(((struct myos_context *)context)->mesa,gl_target,
        (unsigned)(uintptr_t)buffer,layer,attrs.GLTextureLevel,&error);
    if(!image->mesa){free(image);fail(error,"MyOS Mesa image");return 0;}
    image->base.Resource.OnFinalRelease=free_image;display_get(display);return &image->base;
}
static EGLBoolean destroy_image(_EGLDisplay *display,_EGLImage *image)
{(void)display;if(image)_eglPutImage(image);return EGL_TRUE;}
bool MyOSEglGetImage(void *handle,struct st_egl_image *out)
{
    _EGLContext *context=_eglGetCurrentContext();if(!context)return false;
    _EGLDisplay *display=_eglLockDisplay(_eglGetDisplayHandle(context->Resource.Display));
    if(!display)return false;
    _EGLImage *image=display->Initialized?_eglLookupImage((EGLImage)handle,display):0;
    if(image&&out)MyOSMesaTranslateImage(((struct myos_image *)image)->mesa,out);
    bool valid=image!=0;_eglUnlockDisplay(display);return valid;
}
const _EGLDriver _eglDriver={.Initialize=initialize,.Terminate=terminate,
    .CreateContext=create_context,.DestroyContext=destroy_context,.MakeCurrent=make_current,
    .CreateWindowSurface=create_window,
    .CreatePbufferSurface=create_pbuffer,.DestroySurface=destroy_surface,.QuerySurface=_eglQuerySurface,
    /* These core helpers always reject binding our EGL_NO_TEXTURE pbuffers,
     * and release of an unbound image is the defined no-op. Never NULL-call. */
    .BindTexImage=_eglBindTexImage,.ReleaseTexImage=_eglReleaseTexImage,.CopyBuffers=copy_buffers,
    .CreateSyncKHR=create_sync,.DestroySyncKHR=destroy_sync,
    .ClientWaitSyncKHR=client_wait_sync,.WaitSyncKHR=server_wait_sync,
    .CreateImageKHR=create_image,.DestroyImageKHR=destroy_image,
    .SwapBuffers=swap_buffers,.SwapInterval=swap_interval,.WaitClient=wait_client,.WaitNative=wait_native};
