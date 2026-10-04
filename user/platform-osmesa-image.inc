/* Native adapter to actual Mesa texture/renderbuffer resource ownership.
 * Validation/reference flow follows Mesa's stw_image.c, whose notice follows:
 * Copyright (c) Microsoft Corporation
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the Software),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions: the above copyright notice
 * and this permission notice shall be included in all copies or substantial
 * portions of the Software. THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY
 * OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR ITS USE OR OTHER DEALINGS.
 */
#include <EGL/egl.h>
#include "main/texobj.h"
#include "main/renderbuffer.h"
#include "state_tracker/st_texture.h"
#include "state_tracker/st_cb_texture.h"
struct myos_osmesa_image {struct pipe_resource *texture;unsigned level,layer;};
bool MyOSEglGetImage(void *,struct st_egl_image *);
int MyOSMesaInitializeImages(void);
void *MyOSMesaCreateImage(OSMesaContext,unsigned,unsigned,unsigned,int,int *);
void MyOSMesaDestroyImage(void *);
void MyOSMesaTranslateImage(void *,struct st_egl_image *);
static bool get_native_image(struct pipe_frontend_screen *screen,void *image,struct st_egl_image *out)
{(void)screen;return MyOSEglGetImage(image,out);}
static bool validate_native_image(struct pipe_frontend_screen *screen,void *image)
{(void)screen;return MyOSEglGetImage(image,NULL);}
static void install_native_images(void)
{
    global_fscreen->get_egl_image=get_native_image;
    global_fscreen->validate_egl_image=validate_native_image;
}
int MyOSMesaInitializeImages(void)
{
    struct pipe_frontend_screen *screen=get_st_manager();
    if(!screen||!screen->screen)return 0;
    static once_flag once=ONCE_FLAG_INIT;call_once(&once,install_native_images);return 1;
}
void *MyOSMesaCreateImage(OSMesaContext context,unsigned target,unsigned name,
                        unsigned layer,int level,int *error)
{
    struct gl_context *gl=context->st->ctx;
    struct pipe_resource *texture=NULL;
    *error=EGL_BAD_PARAMETER;
    if(!name)return NULL;
    if(target==GL_RENDERBUFFER){
        struct gl_renderbuffer *buffer=_mesa_lookup_renderbuffer(gl,name);
        if(!buffer||buffer->NumSamples>0||!buffer->texture)return NULL;
        texture=buffer->texture;layer=0;level=0;
    }else{
        struct gl_texture_object *object=_mesa_lookup_texture(gl,name);
        if(!object||object->Target!=target)return NULL;
        if(level<0||level>=MAX_TEXTURE_LEVELS){*error=EGL_BAD_MATCH;return NULL;}
        unsigned face=target==GL_TEXTURE_CUBE_MAP?layer:0;
        if(face>=6)return NULL;
        _mesa_test_texobj_completeness(gl,object);
        if(!object->_BaseComplete||(level>0&&!object->_MipmapComplete))return NULL;
        if(level<object->Attrib.BaseLevel||level>object->_MaxLevel||!object->Image[face][level]){
            *error=EGL_BAD_MATCH;return NULL;
        }
        if(target==GL_TEXTURE_3D&&layer>=(unsigned)object->Image[0][level]->Depth){
            *error=EGL_BAD_MATCH;return NULL;
        }
        if(!st_finalize_texture(gl,context->st->pipe,object,0)){
            *error=EGL_BAD_ALLOC;return NULL;
        }
        texture=st_get_texobj_resource(object);if(!texture)return NULL;
    }
    struct myos_osmesa_image *image=CALLOC_STRUCT(myos_osmesa_image);
    if(!image){*error=EGL_BAD_ALLOC;return NULL;}
    pipe_resource_reference(&image->texture,texture);
    image->level=(unsigned)level;image->layer=layer;
    gl->Shared->HasExternallySharedImages=true;*error=EGL_SUCCESS;return image;
}
void MyOSMesaDestroyImage(void *handle)
{struct myos_osmesa_image *image=handle;pipe_resource_reference(&image->texture,NULL);FREE(image);}
void MyOSMesaTranslateImage(void *handle,struct st_egl_image *out)
{
    struct myos_osmesa_image *image=handle;
    pipe_resource_reference(&out->texture,image->texture);
    out->format=image->texture->format;out->level=image->level;out->layer=image->layer;
    out->imported_dmabuf=false;
}
