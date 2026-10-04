/* Compile the sealed, MyOS-configured original frontend in this translation
 * unit so the adapter uses its actual state tracker. Not a substitute GL API. */
#include "osmesa.c"
#include "state_tracker/st_cb_flush.h"
#include "main/debug_output.h"
#include "main/viewport.h"
#include "main/scissor.h"
int MyOSMesaMakeCurrentSurfaceless(OSMesaContext);
int MyOSMesaEnableDebug(OSMesaContext);
int MyOSMesaMakeCurrentSurfaceless(OSMesaContext context)
{
    if(!context||!context->st||!st_api_make_current(context->st,NULL,NULL))return 0;
    struct gl_context *gl=context->st->ctx;
    /* Mesa's generic check_init_viewport intentionally waits for nonzero
     * drawable dimensions. EGL first surfaceless bind is initialization too:
     * zero regions must not be reset on the first later surface bind. */
    if(!gl->ViewportInitialized){
        gl->ViewportInitialized=GL_TRUE;
        for(unsigned i=0;i<MAX_VIEWPORTS;++i){
            _mesa_set_viewport(gl,i,0,0,0,0);_mesa_set_scissor(gl,i,0,0,0,0);
        }
    }
    return 1;
}
int MyOSMesaEnableDebug(OSMesaContext context)
{
    /* Same actual Mesa debug state as st_api_create_context's debug path,
     * set before first bind; OSMesa's public constructor has no flag token. */
    if(!_mesa_set_debug_state_int(context->st->ctx,GL_DEBUG_OUTPUT,GL_TRUE))return 0;
    context->st->ctx->Const.ContextFlags|=GL_CONTEXT_FLAG_DEBUG_BIT;
    _mesa_update_debug_callback(context->st->ctx);return 1;
}
struct myos_osmesa_fence {
    struct pipe_screen *screen;
    struct pipe_fence_handle *fence;
};
void *MyOSMesaCreateFence(void);
int MyOSMesaWaitFence(void *,uint64_t);
int MyOSMesaServerWaitFence(void *);
void MyOSMesaDestroyFence(void *);
void *MyOSMesaCreateFence(void)
{
    OSMesaContext context=OSMesaGetCurrentContext();
    if(!context||!context->st)return NULL;
    struct myos_osmesa_fence *result=CALLOC_STRUCT(myos_osmesa_fence);
    if(!result)return NULL;
    result->screen=context->st->screen;
    st_flush(context->st,&result->fence,0);
    if(!result->fence){FREE(result);return NULL;}
    return result;
}
int MyOSMesaWaitFence(void *handle,uint64_t timeout)
{
    struct myos_osmesa_fence *fence=handle;
    return fence->screen->fence_finish(fence->screen,NULL,fence->fence,timeout);
}
int MyOSMesaServerWaitFence(void *handle)
{
    OSMesaContext context=OSMesaGetCurrentContext();
    if(!context||!context->st)return 0;
    struct myos_osmesa_fence *fence=handle;
    struct pipe_context *pipe=context->st->pipe;
    if(pipe->fence_server_sync){pipe->fence_server_sync(pipe,fence->fence);return 1;}
    /* Original softpipe runs synchronously and has no server-sync callback.
     * Check its genuine completion token rather than inventing success. */
    return MyOSMesaWaitFence(handle,0);
}
void MyOSMesaDestroyFence(void *handle)
{
    struct myos_osmesa_fence *fence=handle;
    fence->screen->fence_reference(fence->screen,&fence->fence,NULL);FREE(fence);
}
#include "platform-osmesa-image.inc"
#include "platform-osmesa-surface.inc"
