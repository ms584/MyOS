/* Surface-owned storage using original OSMesa drawable/flush infrastructure.
 * Derived from its validation setup; original notice:
 * Copyright (c) 2013 Brian Paul All Rights Reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the Software),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
void *MyOSMesaCreateDrawable(void *,unsigned,unsigned,int);
void MyOSMesaDestroyDrawable(void *);
int MyOSMesaBindDrawables(OSMesaContext,void *,void *,int);
static bool myos_surface_validate(struct st_context *st,
    struct pipe_frontend_drawable *drawable,const enum st_attachment_type *attachments,
    unsigned count,struct pipe_resource **out,struct pipe_resource **resolve)
{
    (void)st;(void)resolve;
    struct osmesa_buffer *buffer=drawable_to_osbuffer(drawable);
    for(unsigned i=0;i<count;++i){
        if(attachments[i]!=ST_ATTACHMENT_FRONT_LEFT)return false;
        /* Return a reference to persistent surface storage, not a fresh
         * allocation for each context's framebuffer validation. */
        pipe_resource_reference(&out[i],buffer->textures[attachments[i]]);
    }
    return true;
}
void *MyOSMesaCreateDrawable(void *pixels,unsigned width,unsigned height,int srgb)
{
    enum pipe_format format=srgb?PIPE_FORMAT_R8G8B8A8_SRGB:PIPE_FORMAT_R8G8B8A8_UNORM;
    struct pipe_screen *screen=get_st_manager()->screen;
    if(!pixels||!width||!height||!screen->is_format_supported(screen,format,
        PIPE_TEXTURE_RECT,0,0,PIPE_BIND_RENDER_TARGET))return NULL;
    struct osmesa_buffer *buffer=osmesa_create_buffer(format,PIPE_FORMAT_NONE,PIPE_FORMAT_NONE);
    if(!buffer)return NULL;
    struct pipe_resource template={0};
    template.target=PIPE_TEXTURE_RECT;template.format=format;
    template.width0=width;template.height0=height;template.depth0=template.array_size=1;
    template.usage=PIPE_USAGE_DEFAULT;template.bind=PIPE_BIND_RENDER_TARGET;
    buffer->textures[ST_ATTACHMENT_FRONT_LEFT]=screen->resource_create(screen,&template);
    if(!buffer->textures[ST_ATTACHMENT_FRONT_LEFT]){osmesa_destroy_buffer(buffer);return NULL;}
    buffer->base.validate=myos_surface_validate;buffer->width=width;buffer->height=height;
    buffer->map=pixels;return buffer;
}
void MyOSMesaDestroyDrawable(void *handle)
{
    struct osmesa_buffer *buffer=handle;if(!buffer)return;
    pipe_resource_reference(&buffer->textures[ST_ATTACHMENT_FRONT_LEFT],NULL);
    osmesa_destroy_buffer(buffer);
}
int MyOSMesaBindDrawables(OSMesaContext context,void *draw_handle,void *read_handle,int y_up)
{
    struct osmesa_buffer *draw=draw_handle,*read=read_handle;
    OSMesaContext old=OSMesaGetCurrentContext();
    if(!context){
        if(draw||read||!st_api_make_current(NULL,NULL,NULL))return 0;
        if(old)old->current_buffer=NULL;
        return 1;
    }
    if(!draw&&!read){
        if(!MyOSMesaMakeCurrentSurfaceless(context))return 0;
    }else{
        if(!draw||!read||!st_api_make_current(context->st,&draw->base,&read->base))return 0;
    }
    /* Borrowed drawable: EGL binding references keep it alive. The context
     * never owns/frees this surface's drawable. No OSMesa postprocessing
     * filters are configured through this production EGL adapter. */
    if(old&&old!=context)old->current_buffer=NULL;
    context->current_buffer=draw;context->type=GL_UNSIGNED_BYTE;
    context->y_up=y_up;context->ever_used=true;return 1;
}
