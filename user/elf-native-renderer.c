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
extern int MyOSUiPresent(void *,const struct native_ui_present *);
extern int MyOSUiPoll(void *,struct native_ui_event *,int);
static SbWindow window;
static unsigned char pixels[32*32*4];
static unsigned char texture_pixels[64*64*4],readback[32*32*4];
static void say(const char *text){write(1,text,strlen(text));}
#define CHECK(value) do{if(!(value)){say("Native renderer FAIL: " #value "\n");__builtin_trap();}}while(0)
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
    CHECK(!OSMesaGetCurrentContext());
    const int attributes[]={OSMESA_FORMAT,OSMESA_RGBA,OSMESA_DEPTH_BITS,0,
        OSMESA_STENCIL_BITS,0,OSMESA_ACCUM_BITS,0,OSMESA_PROFILE,OSMESA_COMPAT_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION,2,OSMESA_CONTEXT_MINOR_VERSION,1,0};
    OSMesaContext context=OSMesaCreateContextAttribs(attributes,0);CHECK(context);
    CHECK(OSMesaMakeCurrent(context,pixels,GL_UNSIGNED_BYTE,32,32));
    CHECK(OSMesaGetCurrentContext()==context);
    say("Native OSMesa context bound\n");
    glViewport(0,0,32,32);glDisable(GL_DITHER);
    glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    unsigned char sample[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sample);
    CHECK(sample[0]==0&&sample[1]==0&&sample[2]==255&&sample[3]==255);
    say("Native OSMesa clear pixels checked\n");
    GLuint vertex=shader(GL_VERTEX_SHADER,"#version 120\nattribute vec2 position;varying vec2 uv;"
        "void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}");
    say("Native OSMesa vertex compiled\n");
    GLuint fragment=shader(GL_FRAGMENT_SHADER,"#version 120\nuniform sampler2D image;varying vec2 uv;"
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
    for(unsigned i=0;i<4;++i){glReadPixels(i&1?24:4,i&2?24:4,1,1,
        GL_RGBA,GL_UNSIGNED_BYTE,sample);CHECK(!memcmp(sample,colors+4*i,4));}
    /* 64 source tiles exceed the 16-entry cache; cross boundaries/evict and
     * then re-upload the original texture to prove invalidation too. */
    for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){
        unsigned i=(y*64+x)*4;texture_pixels[i]=(unsigned char)(x*4);
        texture_pixels[i+1]=(unsigned char)(y*4);
        texture_pixels[i+2]=(unsigned char)(((x/8+y/8)&1)*255);texture_pixels[i+3]=255;}
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,texture_pixels);
    glDrawArrays(GL_TRIANGLES,0,6);glFinish();
    glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,readback);
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x){
        unsigned i=(y*32+x)*4;CHECK(readback[i]==x*8+4&&readback[i+1]==y*8+4&&
            readback[i+2]==((x/4+y/4)&1)*255&&readback[i+3]==255);}
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,colors);
    glDrawArrays(GL_TRIANGLES,0,6);glFinish();CHECK(glGetError()==GL_NO_ERROR);
    say("Native OSMesa shaders/textures/pixels checked\n");
    glDisableVertexAttribArray((GLuint)attribute);glUseProgram(0);
    glDeleteBuffers(1,&buffer);glDeleteTextures(1,&texture);
    glDeleteProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);
    struct native_ui_present present={(uintptr_t)pixels,32,32,128,NATIVE_UI_RGBA8,688,520};
    CHECK(!MyOSUiPresent(window,&present));
    CHECK(OSMesaMakeCurrent(0,0,GL_UNSIGNED_BYTE,0,0));
    CHECK(!OSMesaGetCurrentContext());OSMesaDestroyContext(context);
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
    say("Native OSMesa frame ready\n");
    struct native_ui_event event;
    do{CHECK(MyOSUiPoll(window,&event,-1)==1);}while(event.type!=NATIVE_UI_KEY_DOWN||event.character!=97);
    CHECK(SbWindowDestroy(window));
    say("Native OSMesa shaders/textures/TLS/present/input/teardown PASS\n");return 90;
}
