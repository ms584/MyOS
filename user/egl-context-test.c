/* Actual requested GLES/debug/surfaceless semantics, not version-string mocks. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string.h>
#include <stdio.h>
int MyOSEglContextTest(EGLDisplay,EGLConfig);
#define CONTEXT_CHECK(value) do{if(!(value)){fprintf(stderr,"Native EGL context FAIL line %d: %s\n",__LINE__,#value);return __LINE__;}}while(0)
static unsigned messages;
static void APIENTRY debug_message(GLenum source,GLenum type,GLuint id,GLenum severity,
                                  GLsizei length,const GLchar *message,const void *user)
{
    if(source==GL_DEBUG_SOURCE_APPLICATION&&type==GL_DEBUG_TYPE_MARKER&&id==37&&
       severity==GL_DEBUG_SEVERITY_HIGH&&length==4&&user==&messages&&
       !memcmp(message,"MyOS",4))++messages;
}
int MyOSEglContextTest(EGLDisplay display,EGLConfig config)
{
    EGLint renderable=0;CONTEXT_CHECK(eglGetConfigAttrib(display,config,EGL_RENDERABLE_TYPE,&renderable));
    CONTEXT_CHECK((renderable&(EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT_KHR))==
                  (EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT_KHR));
    const EGLint bad_versions[][5]={{EGL_CONTEXT_MAJOR_VERSION_KHR,2,EGL_CONTEXT_MINOR_VERSION_KHR,1,EGL_NONE},
        {EGL_CONTEXT_MAJOR_VERSION_KHR,3,EGL_CONTEXT_MINOR_VERSION_KHR,2,EGL_NONE},
        {EGL_CONTEXT_MAJOR_VERSION_KHR,3,EGL_CONTEXT_MINOR_VERSION_KHR,-1,EGL_NONE}};
    for(unsigned i=0;i<3;++i){
        CONTEXT_CHECK(eglCreateContext(display,config,EGL_NO_CONTEXT,bad_versions[i])==EGL_NO_CONTEXT);
        CONTEXT_CHECK(eglGetError()==EGL_BAD_MATCH);
    }
    const EGLint bad_flags[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_CONTEXT_FLAGS_KHR,0x80,EGL_NONE};
    CONTEXT_CHECK(eglCreateContext(display,config,EGL_NO_CONTEXT,bad_flags)==EGL_NO_CONTEXT&&eglGetError()==EGL_BAD_ATTRIBUTE);
    const EGLint forward[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR,EGL_NONE};
    CONTEXT_CHECK(eglCreateContext(display,config,EGL_NO_CONTEXT,forward)==EGL_NO_CONTEXT&&eglGetError()==EGL_BAD_ATTRIBUTE);
    const EGLint robust[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR,EGL_NONE};
    CONTEXT_CHECK(eglCreateContext(display,config,EGL_NO_CONTEXT,robust)==EGL_NO_CONTEXT&&eglGetError()==EGL_BAD_ATTRIBUTE);
    const EGLint attributes[]={EGL_CONTEXT_MAJOR_VERSION_KHR,3,EGL_CONTEXT_MINOR_VERSION_KHR,1,
        EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,attributes);
    CONTEXT_CHECK(context!=EGL_NO_CONTEXT);
    EGLint requested=0;CONTEXT_CHECK(eglQueryContext(display,context,EGL_CONTEXT_CLIENT_VERSION,&requested)&&requested==3);
    CONTEXT_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    CONTEXT_CHECK(eglGetCurrentContext()==context&&eglGetCurrentDisplay()==display&&
        eglGetCurrentSurface(EGL_DRAW)==EGL_NO_SURFACE&&eglGetCurrentSurface(EGL_READ)==EGL_NO_SURFACE);
    GLint version=0,minor=0,viewport[4],scissor[4];
    glGetIntegerv(GL_MAJOR_VERSION,&version);glGetIntegerv(GL_MINOR_VERSION,&minor);
    glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);
    CONTEXT_CHECK(version==3&&minor==1&&glGetError()==GL_NO_ERROR);
    /* GL_CONTEXT_FLAGS is only a GLES3.2 query in this original Mesa profile.
     * Check its rejection, then prove debug through real state/callbacks. */
    GLint flags=91;glGetIntegerv(GL_CONTEXT_FLAGS,&flags);
    CONTEXT_CHECK(glGetError()==GL_INVALID_ENUM&&flags==91);
    CONTEXT_CHECK(!viewport[0]&&!viewport[1]&&!viewport[2]&&!viewport[3]);
    CONTEXT_CHECK(!scissor[0]&&!scissor[1]&&!scissor[2]&&!scissor[3]);
    CONTEXT_CHECK(glIsEnabled(GL_DEBUG_OUTPUT));
    PFNGLDEBUGMESSAGECALLBACKPROC callback=(PFNGLDEBUGMESSAGECALLBACKPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
    PFNGLDEBUGMESSAGEINSERTPROC insert=(PFNGLDEBUGMESSAGEINSERTPROC)eglGetProcAddress("glDebugMessageInsertKHR");
    CONTEXT_CHECK(callback&&insert);glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);callback(debug_message,&messages);
    unsigned before=messages;insert(GL_DEBUG_SOURCE_APPLICATION,GL_DEBUG_TYPE_MARKER,37,GL_DEBUG_SEVERITY_HIGH,4,"MyOS");
    CONTEXT_CHECK(messages==before+1&&glGetError()==GL_NO_ERROR);
    CONTEXT_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_UNDEFINED);
    glClear(GL_COLOR_BUFFER_BIT);CONTEXT_CHECK(glGetError()==GL_INVALID_FRAMEBUFFER_OPERATION);
    /* A context with no EGL surface can still render to a genuine user FBO. */
    GLuint texture,framebuffer;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
    glGenFramebuffers(1,&framebuffer);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    CONTEXT_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glViewport(0,0,2,2);glClearColor(1,0,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    unsigned char pixels[16];glReadPixels(0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    const unsigned char magenta[]={255,0,255,255};
    for(unsigned i=0;i<4;++i)CONTEXT_CHECK(!memcmp(pixels+i*4,magenta,4));
    CONTEXT_CHECK(glGetError()==GL_NO_ERROR);
    const EGLint pbuffer_attributes[]={EGL_WIDTH,4,EGL_HEIGHT,4,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,pbuffer_attributes);CONTEXT_CHECK(surface!=EGL_NO_SURFACE);
    CONTEXT_CHECK(!eglMakeCurrent(display,surface,EGL_NO_SURFACE,context)&&eglGetError()==EGL_BAD_MATCH);
    CONTEXT_CHECK(eglGetCurrentContext()==context&&eglGetCurrentSurface(EGL_DRAW)==EGL_NO_SURFACE);
    CONTEXT_CHECK(eglMakeCurrent(display,surface,surface,context));
    glBindFramebuffer(GL_FRAMEBUFFER,0);CONTEXT_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    CONTEXT_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context));
    CONTEXT_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_UNDEFINED);
    /* Previously-set viewport remains intact after later bindings. */
    glGetIntegerv(GL_VIEWPORT,viewport);CONTEXT_CHECK(viewport[2]==2&&viewport[3]==2);
    glDeleteFramebuffers(1,&framebuffer);glDeleteTextures(1,&texture);callback(0,0);
    CONTEXT_CHECK(glGetError()==GL_NO_ERROR);
    CONTEXT_CHECK(eglDestroyContext(display,context)); /* deferred while current */
    CONTEXT_CHECK(eglGetCurrentContext()==context);
    CONTEXT_CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    CONTEXT_CHECK(eglDestroySurface(display,surface));return 0;
}
