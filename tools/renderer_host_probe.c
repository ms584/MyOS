/* Host-only Mesa feasibility measurement. Never a MyOS/Cobalt acceptance test. */
#define GL_GLEXT_PROTOTYPES
#include <GL/osmesa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char *message)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static GLuint shader(GLenum type, const char *source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, NULL); glCompileShader(result);
    GLint okay = 0; glGetShaderiv(result, GL_COMPILE_STATUS, &okay);
    if (!okay) { char log[2048]; glGetShaderInfoLog(result, sizeof(log), NULL, log); fprintf(stderr, "%s\n", log); }
    require(okay, "shader compilation"); return result;
}
int main(void)
{
    const int attributes[] = {OSMESA_FORMAT, OSMESA_RGBA, OSMESA_PROFILE,
        OSMESA_COMPAT_PROFILE, OSMESA_CONTEXT_MAJOR_VERSION, 3,
        OSMESA_CONTEXT_MINOR_VERSION, 3, 0};
    OSMesaContext context = OSMesaCreateContextAttribs(attributes, NULL);
    require(context != NULL, "OSMesa context");
    unsigned char guarded[64 * 64 * 4 + 64]; memset(guarded, 0xa5, sizeof(guarded));
    unsigned char *pixels = guarded + 32;
    require(OSMesaMakeCurrent(context, pixels, GL_UNSIGNED_BYTE, 64, 64), "make current");
    OSMesaPixelStore(OSMESA_Y_UP, 1);
    printf("HOST ONLY: renderer=%s; GL=%s; GLSL=%s\n", glGetString(GL_RENDERER),
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLuint vertex = shader(GL_VERTEX_SHADER,
        "#version 120\nattribute vec2 position; attribute vec2 uv; varying vec2 tex;"
        "void main(){tex=uv;gl_Position=vec4(position,0.0,1.0);}");
    GLuint fragment = shader(GL_FRAGMENT_SHADER,
        "#version 120\nuniform sampler2D image;varying vec2 tex;"
        "void main(){gl_FragColor=texture2D(image,tex);}");
    GLuint program = glCreateProgram(); glAttachShader(program, vertex); glAttachShader(program, fragment);
    glBindAttribLocation(program, 0, "position"); glBindAttribLocation(program, 1, "uv"); glLinkProgram(program);
    GLint linked; glGetProgramiv(program, GL_LINK_STATUS, &linked); require(linked, "shader link");
    glUseProgram(program); glUniform1i(glGetUniformLocation(program, "image"), 0);
    const float quad[] = {-1,-1,0,0, 1,-1,1,0, -1,1,0,1, 1,1,1,1};
    GLuint buffer, texture; glGenBuffers(1, &buffer); glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const unsigned char colors[] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);
    glViewport(0, 0, 64, 64); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glFinish();
    const int positions[] = {8 + 8*64, 56 + 8*64, 8 + 56*64, 56 + 56*64};
    for (int i = 0; i < 4; ++i) require(!memcmp(pixels + positions[i]*4, colors + i*4, 4), "texture quadrant pixels");
    /* Known glyph mask: exercise sampled alpha and blending, not font/DOM support. */
    const unsigned char glyph[8] = {0x18,0x24,0x42,0x42,0x7e,0x42,0x42,0x00};
    unsigned char atlas[8*8*4];
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) {
        int p = (y*8+x)*4; atlas[p]=atlas[p+1]=atlas[p+2]=255;
        atlas[p+3]=(glyph[y] & (0x80 >> x)) ? 255 : 0;
    }
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); glFinish();
    for (int y=0; y<8; ++y) for (int x=0; x<8; ++x) {
        unsigned char *p=pixels+((y*8+4)*64+x*8+4)*4;
        int expected=(glyph[y] & (0x80 >> x)) ? 255 : 0;
        require(p[0]==expected && p[1]==expected && p[2]==expected && p[3]==255, "glyph mask pixels");
    }
    require(glGetError()==GL_NO_ERROR, "GL error state");
    for (int i=0; i<32; ++i) require(guarded[i]==0xa5 && guarded[sizeof(guarded)-32+i]==0xa5, "buffer guards");
    glDeleteTextures(1, &texture); glDeleteBuffers(1, &buffer);
    glDeleteProgram(program); glDeleteShader(vertex); glDeleteShader(fragment);
    OSMesaDestroyContext(context);
    puts("PASS: HOST ONLY context/shader/texture/glyph-mask/guards; native MyOS gate NOT tested");
    return 0;
}
