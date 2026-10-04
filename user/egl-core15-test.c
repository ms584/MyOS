/* Core EGLAttrib APIs backed by genuine Mesa objects, no OpenCL capability. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "starboard/egl.h"
/* Exercise the exact pinned table ABI, not a parallel fixture-only layout. */
#define eglQueryString interface->eglQueryString
#define eglGetError interface->eglGetError
#define eglCreateSync interface->eglCreateSync
#define eglGetProcAddress interface->eglGetProcAddress
#define eglCreateContext interface->eglCreateContext
#define eglMakeCurrent interface->eglMakeCurrent
#define eglWaitSync interface->eglWaitSync
#define eglClientWaitSync interface->eglClientWaitSync
#define eglGetSyncAttrib interface->eglGetSyncAttrib
#define eglCreateImage interface->eglCreateImage
#define eglDestroyImage interface->eglDestroyImage
#define eglDestroySync interface->eglDestroySync
#define eglDestroyContext interface->eglDestroyContext
int MyOSEglCore15Test(EGLDisplay,EGLConfig);
#define CORE_CHECK(x) do{if(!(x)){fprintf(stderr,"Native EGL core15 FAIL line %d: %s\n",__LINE__,#x);return __LINE__;}}while(0)
int MyOSEglCore15Test(EGLDisplay display,EGLConfig config)
{
    const SbEglInterface *interface=SbGetEglInterface();CORE_CHECK(interface);
    CORE_CHECK(interface==SbGetEglInterface()&&interface->eglGetDisplay(0)==display);
    for(unsigned i=0;i<44;++i){
        void (*slot)(void)=NULL;memcpy(&slot,(const char *)interface+i*sizeof(slot),sizeof(slot));
        CORE_CHECK(slot);
    }
    CORE_CHECK(!strncmp(eglQueryString(display,EGL_VERSION),"1.5",3));
    CORE_CHECK(!strstr(eglQueryString(display,EGL_EXTENSIONS),"EGL_KHR_cl_event2"));
    CORE_CHECK(eglCreateSync(display,EGL_SYNC_FENCE,NULL)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_MATCH);
    CORE_CHECK(eglCreateSync(display,EGL_SYNC_CL_EVENT,NULL)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_PARAMETER);
    const EGLAttrib cl_event[]={EGL_CL_EVENT_HANDLE,(EGLAttrib)UINT64_C(0x100000001),EGL_NONE};
    CORE_CHECK(eglCreateSync(display,EGL_SYNC_CL_EVENT,cl_event)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_PARAMETER);
    PFNEGLCREATESYNC64KHRPROC khr64=(PFNEGLCREATESYNC64KHRPROC)eglGetProcAddress("eglCreateSync64KHR");
    CORE_CHECK(khr64&&khr64(display,EGL_SYNC_FENCE,NULL)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_MATCH);
    const EGLint options[]={EGL_CONTEXT_MAJOR_VERSION,3,EGL_CONTEXT_MINOR_VERSION,0,
        EGL_CONTEXT_OPENGL_DEBUG,EGL_TRUE,EGL_CONTEXT_OPENGL_ROBUST_ACCESS,EGL_FALSE,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,options);CORE_CHECK(context!=EGL_NO_CONTEXT);
    CORE_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    CORE_CHECK(glIsEnabled(GL_DEBUG_OUTPUT));
    CORE_CHECK(khr64(display,EGL_SYNC_FENCE,NULL)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_MATCH);
    const EGLAttrib bad_attributes[]={EGL_WIDTH,1,EGL_NONE};
    CORE_CHECK(eglCreateSync(display,EGL_SYNC_FENCE,bad_attributes)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_ATTRIBUTE);
    const EGLAttrib wide_attribute[]={(EGLAttrib)UINT64_C(0x100003038),1,EGL_NONE};
    CORE_CHECK(eglCreateSync(display,EGL_SYNC_FENCE,wide_attribute)==EGL_NO_SYNC&&eglGetError()==EGL_BAD_ATTRIBUTE);
    GLuint texture,framebuffer;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    CORE_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glClearColor(0,1,1,1);glClear(GL_COLOR_BUFFER_BIT);
    EGLSync sync=eglCreateSync(display,EGL_SYNC_FENCE,NULL);CORE_CHECK(sync!=EGL_NO_SYNC);
    CORE_CHECK(eglWaitSync(display,sync,0));
    CORE_CHECK(eglClientWaitSync(display,sync,EGL_SYNC_FLUSH_COMMANDS_BIT,EGL_FOREVER)==EGL_CONDITION_SATISFIED);
    EGLAttrib value=-1;CORE_CHECK(eglGetSyncAttrib(display,sync,EGL_SYNC_TYPE,&value)&&value==EGL_SYNC_FENCE);
    value=0;CORE_CHECK(eglGetSyncAttrib(display,sync,EGL_SYNC_STATUS,&value)&&value==EGL_SIGNALED);
    value=(EGLAttrib)UINT64_C(0x100000001);
    CORE_CHECK(!eglGetSyncAttrib(display,sync,EGL_NONE,&value)&&eglGetError()==EGL_BAD_ATTRIBUTE&&
        value==(EGLAttrib)UINT64_C(0x100000001));
    unsigned char pixels[16];glReadPixels(0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    const unsigned char cyan[]={0,255,255,255};
    for(unsigned i=0;i<4;++i)CORE_CHECK(!memcmp(pixels+i*4,cyan,4));
    const EGLAttrib image_options[]={EGL_IMAGE_PRESERVED,EGL_TRUE,EGL_NONE};
    const EGLAttrib wide_image_options[]={(EGLAttrib)UINT64_C(0x1000030d2),EGL_TRUE,EGL_NONE};
    CORE_CHECK(eglCreateImage(display,context,EGL_GL_TEXTURE_2D,(EGLClientBuffer)(uintptr_t)texture,
        wide_image_options)==EGL_NO_IMAGE&&eglGetError()==EGL_BAD_PARAMETER);
    EGLImage image=eglCreateImage(display,context,EGL_GL_TEXTURE_2D,(EGLClientBuffer)(uintptr_t)texture,image_options);
    CORE_CHECK(image!=EGL_NO_IMAGE&&eglDestroyImage(display,image));
    CORE_CHECK(!eglDestroyImage(display,image)&&eglGetError()==EGL_BAD_PARAMETER);
    CORE_CHECK(eglDestroySync(display,sync));
    CORE_CHECK(!eglDestroySync(display,sync)&&eglGetError()==EGL_BAD_PARAMETER);
    glDeleteFramebuffers(1,&framebuffer);glDeleteTextures(1,&texture);CORE_CHECK(glGetError()==GL_NO_ERROR);
    CORE_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    CORE_CHECK(eglDestroyContext(display,context));return 0;
}
