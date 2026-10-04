/* Actual colorspace pixels, distinct draw/read and surface-owned storage. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/osmesa.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string.h>
#include <stdio.h>
int MyOSEglSurfaceTest(EGLDisplay,EGLConfig);
#define SURFACE_CHECK(value) do{if(!(value)){fprintf(stderr,"Native EGL surface FAIL line %d: %s\n",__LINE__,#value);return __LINE__;}}while(0)
static int color(unsigned r,unsigned g,unsigned b,unsigned a)
{
    unsigned char pixels[16];glReadPixels(0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if(glGetError()!=GL_NO_ERROR)return 0;
    for(unsigned i=0;i<4;++i)if(pixels[i*4]!=r||pixels[i*4+1]!=g||pixels[i*4+2]!=b||pixels[i*4+3]!=a){
        fprintf(stderr,"Native EGL surface actual pixel %u,%u,%u,%u expected %u,%u,%u,%u\n",
            pixels[i*4],pixels[i*4+1],pixels[i*4+2],pixels[i*4+3],r,g,b,a);return 0;
    }
    return 1;
}
int MyOSEglSurfaceTest(EGLDisplay display,EGLConfig config)
{
    const EGLint linear_attributes[]={EGL_WIDTH,2,EGL_HEIGHT,2,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_LINEAR_KHR,EGL_NONE};
    const EGLint srgb_attributes[]={EGL_WIDTH,2,EGL_HEIGHT,2,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
    const EGLint invalid[]={EGL_GL_COLORSPACE_KHR,1,EGL_NONE};
    SURFACE_CHECK(eglCreatePbufferSurface(display,config,invalid)==EGL_NO_SURFACE&&eglGetError()==EGL_BAD_ATTRIBUTE);
    EGLSurface linear=eglCreatePbufferSurface(display,config,linear_attributes);
    EGLSurface srgb=eglCreatePbufferSurface(display,config,srgb_attributes);
    SURFACE_CHECK(linear!=EGL_NO_SURFACE&&srgb!=EGL_NO_SURFACE);
    EGLint space=0;SURFACE_CHECK(eglQuerySurface(display,linear,EGL_GL_COLORSPACE_KHR,&space)&&space==EGL_GL_COLORSPACE_LINEAR_KHR);
    SURFACE_CHECK(eglQuerySurface(display,srgb,EGL_GL_COLORSPACE_KHR,&space)&&space==EGL_GL_COLORSPACE_SRGB_KHR);
    const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext first=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
    EGLContext second=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
    SURFACE_CHECK(first!=EGL_NO_CONTEXT&&second!=EGL_NO_CONTEXT);
    SURFACE_CHECK(eglMakeCurrent(display,linear,linear,first));
    GLint encoding=0;glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_BACK,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&encoding);
    SURFACE_CHECK(encoding==GL_LINEAR&&glGetError()==GL_NO_ERROR);
    glClearColor(.5f,.5f,.5f,.5f);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    SURFACE_CHECK(color(128,128,128,128));
    SURFACE_CHECK(eglMakeCurrent(display,srgb,srgb,first));
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_BACK,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&encoding);
    SURFACE_CHECK(encoding==GL_SRGB&&glGetError()==GL_NO_ERROR);
    glClear(GL_COLOR_BUFFER_BIT);glFinish();SURFACE_CHECK(color(188,188,188,128));
    /* Distinct draw/read targets: clearing draw must not touch read storage. */
    SURFACE_CHECK(eglMakeCurrent(display,linear,srgb,first));
    SURFACE_CHECK(eglGetCurrentSurface(EGL_DRAW)==linear&&eglGetCurrentSurface(EGL_READ)==srgb);
    glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    SURFACE_CHECK(color(188,188,188,128));
    /* An unshared context sees and changes persistent surface storage. */
    SURFACE_CHECK(eglMakeCurrent(display,srgb,linear,second));
    OSMesaContext backend=OSMesaGetCurrentContext();SURFACE_CHECK(backend);
    SURFACE_CHECK(color(255,0,0,255));
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    SURFACE_CHECK(color(255,0,0,255));
    SURFACE_CHECK(eglMakeCurrent(display,linear,linear,first));SURFACE_CHECK(color(255,0,0,255));
    GLint width=71,height=72,format=73;void *pixels=(void *)74;
    SURFACE_CHECK(!OSMesaGetColorBuffer(backend,&width,&height,&format,&pixels));
    SURFACE_CHECK(eglMakeCurrent(display,srgb,srgb,first));SURFACE_CHECK(color(0,0,255,255));
    SURFACE_CHECK(eglDestroySurface(display,srgb));
    SURFACE_CHECK(eglGetCurrentSurface(EGL_DRAW)==srgb&&eglGetCurrentSurface(EGL_READ)==srgb);
    SURFACE_CHECK(color(0,0,255,255)); /* actual backing survives public deletion */
    space=73;SURFACE_CHECK(!eglQuerySurface(display,srgb,EGL_GL_COLORSPACE_KHR,&space)&&eglGetError()==EGL_BAD_SURFACE&&space==73);
    SURFACE_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    SURFACE_CHECK(eglDestroySurface(display,linear));
    SURFACE_CHECK(eglDestroyContext(display,first)&&eglDestroyContext(display,second));return 0;
}
