/* Genuine Mesa softpipe shaders/textures in Ring 3, not a renderer substitute. */
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <GL/glext.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "window.h"
#include "native-ui.h"
#if defined(MYOS_NATIVE_EGL)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
#if defined(MYOS_NATIVE_GLES)
#define NATIVE_PROFILE OSMESA_MYOS_GLES_PROFILE
#define NATIVE_MINOR 0
#define VERTEX_VERSION "#version 100\n"
#define FRAGMENT_VERSION "#version 100\nprecision mediump float;\n"
#else
#define NATIVE_PROFILE OSMESA_COMPAT_PROFILE
#define NATIVE_MINOR 1
#define VERTEX_VERSION "#version 120\n"
#define FRAGMENT_VERSION "#version 120\n"
#endif
extern int MyOSUiPresent(void *,const struct native_ui_present *);
extern int MyOSUiPoll(void *,struct native_ui_event *,int);
static SbWindow window;
static unsigned char pixels[32*32*4];
static unsigned char texture_pixels[64*64*4],readback[32*32*4];
extern const unsigned char renderer_font[4096];
static void say(const char *text){write(1,text,strlen(text));}
#define CHECK(value) do{if(!(value)){say("Native renderer FAIL: " #value "\n");__builtin_trap();}}while(0)
#if defined(MYOS_NATIVE_EGL)
struct fence_wait {
    EGLDisplay display;EGLSyncKHR sync;
    PFNEGLCREATESYNCKHRPROC create;
    PFNEGLCLIENTWAITSYNCKHRPROC wait;
    PFNEGLGETSYNCATTRIBKHRPROC query;
    PFNEGLWAITSYNCKHRPROC server_wait;
};
static struct fence_wait pending_fence;
static PFNEGLDESTROYSYNCKHRPROC destroy_pending_fence;
static void *wait_fence(void *argument)
{
    struct fence_wait *work=argument;
    CHECK(eglGetCurrentContext()==EGL_NO_CONTEXT&&!OSMesaGetCurrentContext());
    CHECK(work->create(work->display,EGL_SYNC_FENCE_KHR,0)==EGL_NO_SYNC_KHR);
    CHECK(eglGetError()==EGL_BAD_MATCH);
    for(unsigned i=0;i<16;++i){
        CHECK(work->wait(work->display,work->sync,0,0)==EGL_CONDITION_SATISFIED_KHR);
        CHECK(work->wait(work->display,work->sync,EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                         EGL_FOREVER_KHR)==EGL_CONDITION_SATISFIED_KHR);
        EGLint status=0;CHECK(work->query(work->display,work->sync,EGL_SYNC_STATUS_KHR,&status));
        CHECK(status==EGL_SIGNALED_KHR);
    }
    CHECK(!work->server_wait(work->display,work->sync,0)&&eglGetError()==EGL_BAD_MATCH);
    CHECK(eglReleaseThread());return (void *)(uintptr_t)91;
}
static void check_fences(EGLDisplay display)
{
    CHECK(strstr(eglQueryString(display,EGL_EXTENSIONS),"EGL_KHR_fence_sync"));
    CHECK(strstr(eglQueryString(display,EGL_EXTENSIONS),"EGL_KHR_wait_sync"));
    struct fence_wait work={.display=display,
        .create=(PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR"),
        .wait=(PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR"),
        .query=(PFNEGLGETSYNCATTRIBKHRPROC)eglGetProcAddress("eglGetSyncAttribKHR"),
        .server_wait=(PFNEGLWAITSYNCKHRPROC)eglGetProcAddress("eglWaitSyncKHR")};
    PFNEGLDESTROYSYNCKHRPROC destroy=(PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
    CHECK(work.create&&work.wait&&work.query&&work.server_wait&&destroy);
    const EGLint bad_attributes[]={EGL_WIDTH,1,EGL_NONE};
    CHECK(work.create(display,EGL_SYNC_FENCE_KHR,bad_attributes)==EGL_NO_SYNC_KHR&&
          eglGetError()==EGL_BAD_ATTRIBUTE);
    CHECK(work.create(display,EGL_SYNC_REUSABLE_KHR,0)==EGL_NO_SYNC_KHR&&
          eglGetError()==EGL_BAD_ATTRIBUTE);
    /* Queue a real pixel write before the fence, without a prior Finish. */
    glEnable(GL_SCISSOR_TEST);glScissor(0,0,1,1);glClearColor(1,0,1,1);
    glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);
    work.sync=work.create(display,EGL_SYNC_FENCE_KHR,0);CHECK(work.sync!=EGL_NO_SYNC_KHR);
    EGLint value=0;
    CHECK(work.query(display,work.sync,EGL_SYNC_TYPE_KHR,&value)&&value==EGL_SYNC_FENCE_KHR);
    CHECK(work.query(display,work.sync,EGL_SYNC_CONDITION_KHR,&value)&&
          value==EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR);
    value=0x777;CHECK(!work.query(display,work.sync,EGL_NONE,&value)&&
          eglGetError()==EGL_BAD_ATTRIBUTE&&value==0x777);
    CHECK(work.server_wait(display,work.sync,0));
    CHECK(!work.server_wait(display,work.sync,1)&&eglGetError()==EGL_BAD_PARAMETER);
    unsigned char completed[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,completed);
    CHECK(completed[0]==255&&completed[1]==0&&completed[2]==255&&completed[3]==255);
    EGLSyncKHR pinned=work.create(display,EGL_SYNC_FENCE_KHR,0);CHECK(pinned!=EGL_NO_SYNC_KHR);
    extern int MyOSEglSyncLifetimeTest(EGLDisplay,EGLSyncKHR);
    CHECK(!MyOSEglSyncLifetimeTest(display,pinned));
    pending_fence=work;destroy_pending_fence=destroy;
}
static void finish_fences(void)
{
    struct fence_wait work=pending_fence;
    CHECK(eglGetCurrentContext()==EGL_NO_CONTEXT&&!OSMesaGetCurrentContext());
    pthread_t workers[8];
    for(unsigned i=0;i<8;++i)CHECK(!pthread_create(&workers[i],0,wait_fence,&work));
    for(unsigned i=0;i<8;++i){void *result;CHECK(!pthread_join(workers[i],&result)&&
        result==(void *)(uintptr_t)91);}
    CHECK(destroy_pending_fence(work.display,work.sync));
    CHECK(!destroy_pending_fence(work.display,work.sync)&&eglGetError()==EGL_BAD_PARAMETER);
    CHECK(eglTerminate(work.display)&&eglTerminate(work.display));
    say("Native EGL real fence/eight waiters/query/errors/final-pin checked\n");
}
#endif
static GLuint shader(GLenum type,const char *source)
{
    GLuint value=glCreateShader(type);CHECK(value);
    glShaderSource(value,1,&source,0);glCompileShader(value);
    GLint compiled=0;glGetShaderiv(value,GL_COMPILE_STATUS,&compiled);
    if(!compiled){char text[256];GLsizei length=0;
        glGetShaderInfoLog(value,sizeof(text),&length,text);write(1,text,(size_t)length);}
    CHECK(compiled);return value;
}
static void *render(void *unused)
{
    (void)unused;say("Native OSMesa context begin\n");
    int origin_x=0,origin_y=0;
#define READ_PIXELS(x,y,w,h,format,type,data) glReadPixels((x)+origin_x,(y)+origin_y,w,h,format,type,data)
    CHECK(!OSMesaGetCurrentContext());
#if defined(MYOS_NATIVE_EGL)
    CHECK(!eglInitialize((EGLDisplay)(uintptr_t)1,0,0));
    CHECK(eglGetError()==EGL_BAD_DISPLAY&&eglGetError()==EGL_SUCCESS);
    EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(display!=EGL_NO_DISPLAY);
    EGLint major=0,minor=0;CHECK(eglInitialize(display,&major,&minor));
    CHECK(major==1&&minor==4); /* Truthful until image/sync/1.5 contracts exist. */
    CHECK(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint config_attributes[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RED_SIZE,8,EGL_NONE};
    EGLConfig config;EGLint count=0;
    CHECK(eglChooseConfig(display,config_attributes,&config,1,&count)&&count==1);
    extern int MyOSEglLifetimeTest(EGLDisplay,EGLConfig);
    CHECK(!MyOSEglLifetimeTest(display,config));
    say("Native EGL last-core-pin cleanup checked\n");
    extern int MyOSEglContextTest(EGLDisplay,EGLConfig);
    CHECK(!MyOSEglContextTest(display,config));
    say("Native EGL requested-version/debug callback/surfaceless FBO checked\n");
    extern int MyOSEglSurfaceTest(EGLDisplay,EGLConfig);
    CHECK(!MyOSEglSurfaceTest(display,config));
    say("Native EGL linear128/sRGB188/draw-read/surface storage lifetime checked\n");
    CHECK(eglCreateContext(display,(EGLConfig)(uintptr_t)1,EGL_NO_CONTEXT,0)==EGL_NO_CONTEXT);
    CHECK(eglGetError()==EGL_BAD_CONFIG);
    const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
    EGLContext egl_context=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
    CHECK(egl_context!=EGL_NO_CONTEXT);
    const EGLint surface_attributes[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};
#if defined(MYOS_NATIVE_EGL_WINDOW)
    struct native_ui_info info;extern int MyOSUiQuery(void *,struct native_ui_info *);
    CHECK(!MyOSUiQuery(window,&info)&&info.width>=720&&info.height>=568);
    const EGLint window_attributes[]={EGL_GL_COLORSPACE_KHR,
#if defined(MYOS_NATIVE_EGL_SRGB)
        EGL_GL_COLORSPACE_SRGB_KHR,
#else
        EGL_GL_COLORSPACE_LINEAR_KHR,
#endif
        EGL_NONE};
    EGLSurface surface=eglCreateWindowSurface(display,config,
        (EGLNativeWindowType)(uintptr_t)SbWindowGetPlatformHandle(window),window_attributes);
    CHECK(surface!=EGL_NO_SURFACE);
    CHECK(eglCreateWindowSurface(display,config,(EGLNativeWindowType)(uintptr_t)window,0)==EGL_NO_SURFACE&&
          eglGetError()==EGL_BAD_ALLOC); /* one surface per native window */
    origin_x=688;origin_y=(int)info.height-552;
#else
    EGLSurface surface=eglCreatePbufferSurface(display,config,surface_attributes);
#endif
    CHECK(surface!=EGL_NO_SURFACE&&eglMakeCurrent(display,surface,surface,egl_context));
    CHECK(eglGetCurrentDisplay()==display&&eglGetCurrentContext()==egl_context&&
          eglGetCurrentSurface(EGL_DRAW)==surface&&eglGetCurrentSurface(EGL_READ)==surface);
    EGLint width=0;CHECK(eglQuerySurface(display,surface,EGL_WIDTH,&width));
#if defined(MYOS_NATIVE_EGL_WINDOW)
    CHECK(width==(EGLint)info.width);
#else
    CHECK(width==32);
#endif
    EGLSurface plain=eglCreatePbufferSurface(display,config,surface_attributes);
    CHECK(plain!=EGL_NO_SURFACE);
    CHECK(!eglBindTexImage(display,plain,EGL_BACK_BUFFER)&&eglGetError()==EGL_BAD_MATCH);
    CHECK(eglReleaseTexImage(display,plain,EGL_BACK_BUFFER));
    CHECK(!eglCopyBuffers(display,plain,(EGLNativePixmapType)(uintptr_t)1)&&
          eglGetError()==EGL_BAD_NATIVE_PIXMAP);
    CHECK(eglDestroySurface(display,plain));
    const EGLint texture_attributes[]={EGL_WIDTH,1,EGL_HEIGHT,1,
        EGL_TEXTURE_FORMAT,EGL_TEXTURE_RGBA,EGL_TEXTURE_TARGET,EGL_TEXTURE_2D,EGL_NONE};
    CHECK(eglCreatePbufferSurface(display,config,texture_attributes)==EGL_NO_SURFACE&&
          eglGetError()==EGL_BAD_MATCH);
    const EGLint large_attributes[]={EGL_WIDTH,2048,EGL_HEIGHT,1,EGL_NONE};
    CHECK(eglCreatePbufferSurface(display,config,large_attributes)==EGL_NO_SURFACE&&
          eglGetError()==EGL_BAD_ALLOC);
    const EGLint largest_attributes[]={EGL_WIDTH,2048,EGL_HEIGHT,1,
        EGL_LARGEST_PBUFFER,EGL_TRUE,EGL_NONE};
    EGLSurface largest=eglCreatePbufferSurface(display,config,largest_attributes);
    CHECK(largest!=EGL_NO_SURFACE&&eglQuerySurface(display,largest,EGL_WIDTH,&width)&&width==1024);
    CHECK(eglDestroySurface(display,largest));
    OSMesaContext context=OSMesaGetCurrentContext();CHECK(context);
#else
    const int attributes[]={OSMESA_FORMAT,OSMESA_RGBA,OSMESA_DEPTH_BITS,0,
        OSMESA_STENCIL_BITS,0,OSMESA_ACCUM_BITS,0,OSMESA_PROFILE,NATIVE_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION,2,OSMESA_CONTEXT_MINOR_VERSION,NATIVE_MINOR,0};
    OSMesaContext context=OSMesaCreateContextAttribs(attributes,0);CHECK(context);
    CHECK(OSMesaMakeCurrent(context,pixels,GL_UNSIGNED_BYTE,32,32));
#endif
    CHECK(OSMesaGetCurrentContext()==context);
    const char *version=(const char *)glGetString(GL_VERSION);CHECK(version);
    say(version);say("\n");
#if defined(MYOS_NATIVE_GLES)
    CHECK(!strncmp(version,"OpenGL ES ",10));
    /* The actual ES profile rejects desktop GLSL, not just a renamed version. */
    GLuint desktop=glCreateShader(GL_VERTEX_SHADER);CHECK(desktop);
    const char *invalid="#version 120\nvoid main(){gl_Position=vec4(0.0);}";
    glShaderSource(desktop,1,&invalid,0);glCompileShader(desktop);
    GLint accepted=1;glGetShaderiv(desktop,GL_COMPILE_STATUS,&accepted);
    CHECK(!accepted);glDeleteShader(desktop);
    say("Native GLES profile/desktop GLSL rejection checked\n");
#else
    CHECK(strncmp(version,"OpenGL ES ",10));
#endif
    say("Native OSMesa context bound\n");
    glViewport(origin_x,origin_y,32,32);glDisable(GL_DITHER);
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    unsigned char sample[4];READ_PIXELS(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sample);
    CHECK(sample[0]==0&&sample[1]==0&&sample[2]==255&&sample[3]==255);
    say("Native OSMesa clear pixels checked\n");
    GLuint vertex=shader(GL_VERTEX_SHADER,VERTEX_VERSION "attribute vec2 position;varying vec2 uv;"
        "void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}");
    say("Native OSMesa vertex compiled\n");
    GLuint fragment=shader(GL_FRAGMENT_SHADER,FRAGMENT_VERSION "uniform sampler2D image;varying vec2 uv;"
        "void main(){gl_FragColor=texture2D(image,uv);}");
    say("Native OSMesa fragment compiled\n");
    GLuint program=glCreateProgram();CHECK(program);
    glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked);
    say("Native OSMesa program linked\n");
    glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
    GLuint texture;glGenTextures(1,&texture);CHECK(texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    const unsigned char colors[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,0,255};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,colors);
    const GLfloat positions[]={-1,-1,1,-1,1,1,-1,-1,1,1,-1,1};
    GLuint buffer;glGenBuffers(1,&buffer);glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(positions),positions,GL_STATIC_DRAW);
    GLint attribute=glGetAttribLocation(program,"position");CHECK(attribute>=0);
    glVertexAttribPointer((GLuint)attribute,2,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray((GLuint)attribute);glDrawArrays(GL_TRIANGLES,0,6);glFinish();
    CHECK(glGetError()==GL_NO_ERROR);
    for(unsigned i=0;i<4;++i){READ_PIXELS(i&1?24:4,i&2?24:4,1,1,
        GL_RGBA,GL_UNSIGNED_BYTE,sample);CHECK(!memcmp(sample,colors+4*i,4));}
    /* 64 source tiles exceed the 16-entry cache; cross boundaries/evict and
     * then re-upload the original texture to prove invalidation too. */
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){
        unsigned i=(y*64+x)*4;texture_pixels[i]=(unsigned char)(x*4);
        texture_pixels[i+1]=(unsigned char)(y*4);
        texture_pixels[i+2]=(unsigned char)(((x/8+y/8)&1)*255);texture_pixels[i+3]=255;}
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,texture_pixels);
    glDrawArrays(GL_TRIANGLES,0,6);glFinish();
    READ_PIXELS(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,readback);
#if defined(MYOS_NATIVE_EGL_SRGB)
    /* Independent rounded 255*(1.055*((8*i+4)/255)^(1/2.4)-.055).
     * Shader/rasterization quantization may differ by at most one byte. */
    const unsigned char encoded_gradient[]={34,61,79,93,105,115,125,133,141,148,155,162,168,174,180,185,
        190,196,200,205,210,214,219,223,227,231,235,239,243,246,250,254};
#endif
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x){
        unsigned i=(y*32+x)*4;
#if defined(MYOS_NATIVE_EGL_SRGB)
        CHECK(readback[i]+1>=encoded_gradient[x]&&readback[i]<=encoded_gradient[x]+1&&
              readback[i+1]+1>=encoded_gradient[y]&&readback[i+1]<=encoded_gradient[y]+1&&
#else
        CHECK(readback[i]==x*8+4&&readback[i+1]==y*8+4&&
#endif
            readback[i+2]==((x/4+y/4)&1)*255&&readback[i+3]==255);}
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,colors);
    glDrawArrays(GL_TRIANGLES,0,6);glFinish();CHECK(glGetError()==GL_NO_ERROR);
    say("Native OSMesa shaders/textures/pixels checked\n");
    READ_PIXELS(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    struct native_ui_present present={(uintptr_t)pixels,32,32,128,NATIVE_UI_RGBA8,688,520};
#if defined(MYOS_NATIVE_EGL_WINDOW)
    (void)present;CHECK(eglSwapBuffers(display,surface));
#else
    CHECK(!MyOSUiPresent(window,&present));
#endif
    /* Existing licensed glyph masks are expanded in MyOS, then sampled and
     * alpha-blended by real Mesa. No host-generated rendered text/frame. */
    static const unsigned char text[]="MyOS";
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x){
        unsigned i=(y*32+x)*4;
        texture_pixels[i]=texture_pixels[i+1]=texture_pixels[i+2]=255;
        unsigned font_row=y;
#if defined(MYOS_NATIVE_EGL_WINDOW)
        font_row=15-y;
#endif
        texture_pixels[i+3]=(renderer_font[text[x/8]*16+font_row]&(128u>>(x%8)))?128:0;
    }
#if defined(MYOS_NATIVE_EGL_WINDOW)
    origin_y=(int)info.height-568;
#endif
    glViewport(origin_x,origin_y,32,16);glClearColor(0,0,0,1);
    /* Clear only the text rectangle; a native surface keeps the color frame. */
    glEnable(GL_SCISSOR_TEST);glScissor(origin_x,origin_y,32,16);glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,16,0,GL_RGBA,GL_UNSIGNED_BYTE,texture_pixels);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES,0,6);glFinish();CHECK(glGetError()==GL_NO_ERROR);
    READ_PIXELS(0,0,32,16,GL_RGBA,GL_UNSIGNED_BYTE,readback);
    unsigned ink=0;
    for(unsigned character=0;character<4;++character)for(unsigned row=0;row<16;++row)
        for(unsigned column=0;column<8;++column){
            unsigned i=(row*32+character*8+column)*4;
            unsigned font_row=row;
#if defined(MYOS_NATIVE_EGL_WINDOW)
            font_row=15-row;
#endif
#if defined(MYOS_NATIVE_EGL_SRGB)
            unsigned intensity=188;
#else
            unsigned intensity=128;
#endif
            unsigned expected=((renderer_font[text[character]*16+font_row]>>(7-column))&1)*intensity;
            CHECK(readback[i]==expected&&readback[i+1]==expected&&
                  readback[i+2]==expected&&readback[i+3]==255);
            ink+=expected!=0;
        }
    CHECK(ink>32&&ink<256);glDisable(GL_BLEND);
    READ_PIXELS(0,0,32,16,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    present=(struct native_ui_present){(uintptr_t)pixels,32,16,128,NATIVE_UI_RGBA8,688,552};
#if defined(MYOS_NATIVE_EGL_WINDOW)
    CHECK(eglSwapBuffers(display,surface));say("Native EGL window swap checked\n");
#else
    CHECK(!MyOSUiPresent(window,&present));
#endif
    say("Native glyph atlas/alpha blend/all512 text pixels checked\n");
    glDisableVertexAttribArray((GLuint)attribute);glUseProgram(0);
    glDeleteBuffers(1,&buffer);glDeleteTextures(1,&texture);
    glDeleteProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);
#if defined(MYOS_NATIVE_EGL)
    extern int MyOSEglImageTest(EGLDisplay,EGLConfig,EGLContext,EGLSurface);
    CHECK(!MyOSEglImageTest(display,config,egl_context,surface));
    say("Native EGL shared2D/cube6/3Dlayer/renderbuffer/orphan/final-pin checked\n");
    check_fences(display);
    CHECK(eglSwapBuffers(display,surface)&&eglWaitClient());
    CHECK(eglDestroyContext(display,egl_context));
    EGLint value;CHECK(!eglQueryContext(display,egl_context,EGL_CONTEXT_CLIENT_VERSION,&value));
    CHECK(eglGetError()==EGL_BAD_CONTEXT&&OSMesaGetCurrentContext()==context);
    CHECK(eglGetCurrentContext()==egl_context);
    CHECK(eglDestroySurface(display,surface));
    CHECK(!eglQuerySurface(display,surface,EGL_WIDTH,&value));
    CHECK(eglGetError()==EGL_BAD_SURFACE);
    CHECK(eglGetCurrentSurface(EGL_DRAW)==surface&&eglGetCurrentSurface(EGL_READ)==surface);
    CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
    CHECK(eglGetCurrentContext()==EGL_NO_CONTEXT&&!OSMesaGetCurrentContext());
    /* Keep display/fence valid for eight waiters after the creator context and
     * worker are gone; main then has all eight kernel worker slots available. */
    CHECK(eglReleaseThread());
    say("Native EGL core/error/current/deferred destroy checked\n");
#else
    CHECK(OSMesaMakeCurrent(0,0,GL_UNSIGNED_BYTE,0,0));
    CHECK(!OSMesaGetCurrentContext());OSMesaDestroyContext(context);
#endif
    say("Native OSMesa context destroyed\n");return (void *)(uintptr_t)90;
}
int _start(void)
{
    SbWindowOptions options;SbWindowSetDefaultOptions(&options);options.name="Native Mesa";
    window=SbWindowCreate(&options);CHECK(window);
    struct {uint64_t before;pthread_attr_t value;uint64_t after;} attributes;
    memset(&attributes,0xa5,sizeof(attributes));
    CHECK(!pthread_attr_init(&attributes.value));
    CHECK(!pthread_attr_setstacksize(&attributes.value,512*1024));
    size_t stack;CHECK(!pthread_attr_getstacksize(&attributes.value,&stack)&&stack==512*1024);
    pthread_t worker;CHECK(!pthread_create(&worker,&attributes.value,render,0));
    CHECK(!pthread_attr_destroy(&attributes.value));
    CHECK(attributes.before==UINT64_C(0xa5a5a5a5a5a5a5a5)&&attributes.after==attributes.before);
    void *result;CHECK(!pthread_join(worker,&result)&&result==(void *)(uintptr_t)90);
    CHECK(!OSMesaGetCurrentContext());
#if defined(MYOS_NATIVE_EGL)
    finish_fences();
#endif
    say("Native OSMesa frame ready\n");
#if defined(MYOS_NATIVE_EGL)
    CHECK(eglGetCurrentContext()==EGL_NO_CONTEXT&&eglGetError()==EGL_SUCCESS);
    CHECK(eglReleaseThread());
#endif
    struct native_ui_event event;
    do{CHECK(MyOSUiPoll(window,&event,-1)==1);}while(event.type!=NATIVE_UI_KEY_DOWN||event.character!=97);
    CHECK(SbWindowDestroy(window));
    say("Native OSMesa shaders/textures/TLS/present/input/teardown PASS\n");return 90;
}
