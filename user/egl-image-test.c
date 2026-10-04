/* Genuine EGL/GLES image sibling/storage tests in MyOS, not mock rendering. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string.h>
#include <stdio.h>
#include "eglimage.h"
int MyOSEglImageTest(EGLDisplay,EGLConfig,EGLContext,EGLSurface);
#define IMAGE_CHECK(value) do{if(!(value)){fprintf(stderr,"Native EGL image FAIL line %d: %s\n",__LINE__,#value);return __LINE__;}}while(0)
static PFNEGLCREATEIMAGEKHRPROC create;
static PFNEGLDESTROYIMAGEKHRPROC destroy;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC import_texture;
static PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC import_buffer;
static const EGLint preserved[]={EGL_IMAGE_PRESERVED_KHR,EGL_TRUE,EGL_NONE};
static void (*release_image)(_EGLResource *);
static unsigned image_releases;
static void count_release(_EGLResource *resource)
{release_image(resource);++image_releases;}
static int read_color(const unsigned char *color)
{
    unsigned char pixels[16];glReadPixels(0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    if(glGetError()!=GL_NO_ERROR)return 0;
    for(unsigned i=0;i<4;++i)if(memcmp(pixels+i*4,color,4))return 0;
    return 1;
}
static void texture_parameters(GLenum target)
{glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
static int import_and_read(EGLImageKHR image,GLuint texture,GLuint framebuffer,const unsigned char *color)
{
    glBindTexture(GL_TEXTURE_2D,texture);import_texture(GL_TEXTURE_2D,(GLeglImageOES)image);
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);
    texture_parameters(GL_TEXTURE_2D);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    IMAGE_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    IMAGE_CHECK(read_color(color));return 0;
}
static int import_buffer_and_read(EGLImageKHR image,GLuint buffer,GLuint framebuffer,const unsigned char *color)
{
    glBindRenderbuffer(GL_RENDERBUFFER,buffer);import_buffer(GL_RENDERBUFFER,(GLeglImageOES)image);
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,buffer);
    IMAGE_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    IMAGE_CHECK(read_color(color));return 0;
}
int MyOSEglImageTest(EGLDisplay display,EGLConfig config,EGLContext producer,EGLSurface original)
{
    create=(PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    destroy=(PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    import_texture=(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    import_buffer=(PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
    IMAGE_CHECK(create&&destroy&&import_texture&&import_buffer);
    const unsigned char colors[24]={255,0,0,255,0,255,0,255,0,0,255,255,
        255,255,0,255,255,0,255,255,0,255,255,255};
    unsigned char data[32];for(unsigned i=0;i<8;++i)memcpy(data+i*4,colors,4);
    GLuint source;glGenTextures(1,&source);glBindTexture(GL_TEXTURE_2D,source);
    texture_parameters(GL_TEXTURE_2D);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);
    IMAGE_CHECK(create(display,EGL_NO_CONTEXT,EGL_GL_TEXTURE_2D_KHR,(EGLClientBuffer)(uintptr_t)source,0)==EGL_NO_IMAGE_KHR&&eglGetError()==EGL_BAD_CONTEXT);
    IMAGE_CHECK(create(display,producer,EGL_GL_TEXTURE_2D_KHR,0,0)==EGL_NO_IMAGE_KHR&&eglGetError()==EGL_BAD_PARAMETER);
    const EGLint negative[]={EGL_GL_TEXTURE_LEVEL_KHR,-1,EGL_NONE};
    IMAGE_CHECK(create(display,producer,EGL_GL_TEXTURE_2D_KHR,(EGLClientBuffer)(uintptr_t)source,negative)==EGL_NO_IMAGE_KHR&&eglGetError()==EGL_BAD_MATCH);
    EGLImageKHR image=create(display,producer,EGL_GL_TEXTURE_2D_KHR,(EGLClientBuffer)(uintptr_t)source,preserved);
    IMAGE_CHECK(image!=EGL_NO_IMAGE_KHR); /* before any draw/finalization by sampler */
    const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    const EGLint surface_attributes[]={EGL_WIDTH,4,EGL_HEIGHT,4,EGL_NONE};
    EGLContext consumer=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
    EGLSurface surface=eglCreatePbufferSurface(display,config,surface_attributes);
    IMAGE_CHECK(consumer!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE);
    IMAGE_CHECK(eglMakeCurrent(display,surface,surface,consumer));
    GLuint imported,consumer_fb;glGenTextures(1,&imported);glGenFramebuffers(1,&consumer_fb);
    IMAGE_CHECK(!import_and_read(image,imported,consumer_fb,colors));
    glViewport(0,0,2,2);glClearColor(0,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    IMAGE_CHECK(read_color(colors+20));
    IMAGE_CHECK(eglMakeCurrent(display,original,original,producer));
    GLuint producer_fb;glGenFramebuffers(1,&producer_fb);glBindFramebuffer(GL_FRAMEBUFFER,producer_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,source,0);
    IMAGE_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE&&read_color(colors+20));
    /* Producer and consumer do not share GL object namespaces: the changed
     * bytes can only arrive through their common actual Gallium storage. */
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&producer_fb);glDeleteTextures(1,&source);
    IMAGE_CHECK(eglMakeCurrent(display,surface,surface,consumer));
    IMAGE_CHECK(read_color(colors+20));
    IMAGE_CHECK(destroy(display,image));
    IMAGE_CHECK(read_color(colors+20)); /* imported sibling survives image destruction */
    import_texture(GL_TEXTURE_2D,(GLeglImageOES)image);IMAGE_CHECK(glGetError()==GL_INVALID_VALUE);
    IMAGE_CHECK(!destroy(display,image)&&eglGetError()==EGL_BAD_PARAMETER);

    IMAGE_CHECK(eglMakeCurrent(display,original,original,producer));
    GLuint cube;glGenTextures(1,&cube);glBindTexture(GL_TEXTURE_CUBE_MAP,cube);texture_parameters(GL_TEXTURE_CUBE_MAP);
    EGLImageKHR faces[6];
    for(unsigned face=0;face<6;++face){
        for(unsigned i=0;i<4;++i)memcpy(data+i*4,colors+face*4,4);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    }
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);
    for(unsigned face=0;face<6;++face){
        faces[face]=create(display,producer,EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR+face,
            (EGLClientBuffer)(uintptr_t)cube,preserved);IMAGE_CHECK(faces[face]!=EGL_NO_IMAGE_KHR);
    }
    glDeleteTextures(1,&cube);IMAGE_CHECK(eglMakeCurrent(display,surface,surface,consumer));
    GLuint imported_buffer;glGenRenderbuffers(1,&imported_buffer);
    for(unsigned face=0;face<6;++face){
        IMAGE_CHECK(!import_buffer_and_read(faces[face],imported_buffer,consumer_fb,colors+face*4));
        IMAGE_CHECK(destroy(display,faces[face]));
    }

    IMAGE_CHECK(eglMakeCurrent(display,original,original,producer));
    GLuint volume;glGenTextures(1,&volume);glBindTexture(GL_TEXTURE_3D,volume);texture_parameters(GL_TEXTURE_3D);
    for(unsigned i=0;i<8;++i)memcpy(data+i*4,colors+(i<4?0:16),4);
    glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA,2,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);
    const EGLint bad_layer[]={EGL_GL_TEXTURE_ZOFFSET_KHR,2,EGL_NONE};
    IMAGE_CHECK(create(display,producer,EGL_GL_TEXTURE_3D_KHR,(EGLClientBuffer)(uintptr_t)volume,bad_layer)==EGL_NO_IMAGE_KHR&&eglGetError()==EGL_BAD_MATCH);
    const EGLint second_layer[]={EGL_GL_TEXTURE_ZOFFSET_KHR,1,EGL_IMAGE_PRESERVED_KHR,EGL_TRUE,EGL_NONE};
    image=create(display,producer,EGL_GL_TEXTURE_3D_KHR,(EGLClientBuffer)(uintptr_t)volume,second_layer);
    IMAGE_CHECK(image!=EGL_NO_IMAGE_KHR);glDeleteTextures(1,&volume);
    IMAGE_CHECK(eglMakeCurrent(display,surface,surface,consumer));
    IMAGE_CHECK(!import_buffer_and_read(image,imported_buffer,consumer_fb,colors+16));IMAGE_CHECK(destroy(display,image));

    IMAGE_CHECK(eglMakeCurrent(display,original,original,producer));
    GLuint buffer;glGenRenderbuffers(1,&buffer);glBindRenderbuffer(GL_RENDERBUFFER,buffer);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA8,2,2);glGenFramebuffers(1,&producer_fb);
    glBindFramebuffer(GL_FRAMEBUFFER,producer_fb);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,buffer);
    IMAGE_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    image=create(display,producer,EGL_GL_RENDERBUFFER_KHR,(EGLClientBuffer)(uintptr_t)buffer,preserved);
    IMAGE_CHECK(image!=EGL_NO_IMAGE_KHR);
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&producer_fb);glDeleteRenderbuffers(1,&buffer);
    IMAGE_CHECK(eglMakeCurrent(display,surface,surface,consumer));
    IMAGE_CHECK(!import_and_read(image,imported,consumer_fb,colors+8));
    _EGLImage *resource=(_EGLImage *)image;release_image=resource->Resource.OnFinalRelease;
    IMAGE_CHECK(release_image);resource->Resource.OnFinalRelease=count_release;
    unsigned before=image_releases;_eglGetImage(resource);
    IMAGE_CHECK(destroy(display,image)&&image_releases==before);
    IMAGE_CHECK(_eglPutImage(resource)&&image_releases==before+1);
    IMAGE_CHECK(read_color(colors+8));
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&consumer_fb);glDeleteTextures(1,&imported);
    glDeleteRenderbuffers(1,&imported_buffer);
    IMAGE_CHECK(eglMakeCurrent(display,original,original,producer));
    IMAGE_CHECK(eglDestroyContext(display,consumer)&&eglDestroySurface(display,surface));
    IMAGE_CHECK(glGetError()==GL_NO_ERROR);return 0;
}
