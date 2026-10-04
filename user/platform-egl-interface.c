/* Exact pinned Starboard EGL table, backed by real Mesa entry points. */
#include <stdint.h>
#include <EGL/egl.h>
#include "starboard/egl.h"
const SbEglInterface *SbGetEglInterface(void);
_Static_assert(sizeof(SbEglInterface)==44*sizeof(void (*)(void)),"pinned EGL table layout");
_Static_assert(sizeof(SbEglAttrib)==sizeof(EGLAttrib),"EGLAttrib ABI");
_Static_assert(sizeof(SbEglTime)==sizeof(EGLTime),"EGLTime ABI");
_Static_assert(sizeof(SbEglInt32)==sizeof(EGLint),"EGLint ABI");
static SbEglDisplay get_display(SbEglNativeDisplayType native)
{return eglGetDisplay((EGLNativeDisplayType)(uintptr_t)native);}
static SbEglBoolean copy_buffers(SbEglDisplay d,SbEglSurface s,SbEglNativePixmapType p)
{return eglCopyBuffers(d,s,(EGLNativePixmapType)(uintptr_t)p);}
static SbEglSurface create_pixmap(SbEglDisplay d,SbEglConfig c,SbEglNativePixmapType p,const SbEglInt32 *a)
{return eglCreatePixmapSurface(d,c,(EGLNativePixmapType)(uintptr_t)p,a);}
static SbEglSurface create_window(SbEglDisplay d,SbEglConfig c,SbEglNativeWindowType w,const SbEglInt32 *a)
{return eglCreateWindowSurface(d,c,(EGLNativeWindowType)(uintptr_t)w,a);}
static const SbEglInterface interface={
    .eglChooseConfig=eglChooseConfig,.eglCopyBuffers=copy_buffers,
    .eglCreateContext=eglCreateContext,.eglCreatePbufferSurface=eglCreatePbufferSurface,
    .eglCreatePixmapSurface=create_pixmap,.eglCreateWindowSurface=create_window,
    .eglDestroyContext=eglDestroyContext,.eglDestroySurface=eglDestroySurface,
    .eglGetConfigAttrib=eglGetConfigAttrib,.eglGetConfigs=eglGetConfigs,
    .eglGetCurrentDisplay=eglGetCurrentDisplay,.eglGetCurrentSurface=eglGetCurrentSurface,
    .eglGetDisplay=get_display,.eglGetError=eglGetError,.eglGetProcAddress=eglGetProcAddress,
    .eglInitialize=eglInitialize,.eglMakeCurrent=eglMakeCurrent,.eglQueryContext=eglQueryContext,
    .eglQueryString=eglQueryString,.eglQuerySurface=eglQuerySurface,
    .eglSwapBuffers=eglSwapBuffers,.eglTerminate=eglTerminate,
    .eglWaitGL=eglWaitGL,.eglWaitNative=eglWaitNative,.eglBindTexImage=eglBindTexImage,
    .eglReleaseTexImage=eglReleaseTexImage,.eglSurfaceAttrib=eglSurfaceAttrib,
    .eglSwapInterval=eglSwapInterval,.eglBindAPI=eglBindAPI,.eglQueryAPI=eglQueryAPI,
    .eglCreatePbufferFromClientBuffer=eglCreatePbufferFromClientBuffer,
    .eglReleaseThread=eglReleaseThread,.eglWaitClient=eglWaitClient,
    .eglGetCurrentContext=eglGetCurrentContext,.eglCreateSync=eglCreateSync,
    .eglDestroySync=eglDestroySync,.eglClientWaitSync=eglClientWaitSync,
    .eglGetSyncAttrib=eglGetSyncAttrib,.eglCreateImage=eglCreateImage,
    .eglDestroyImage=eglDestroyImage,.eglGetPlatformDisplay=eglGetPlatformDisplay,
    .eglCreatePlatformWindowSurface=eglCreatePlatformWindowSurface,
    .eglCreatePlatformPixmapSurface=eglCreatePlatformPixmapSurface,.eglWaitSync=eglWaitSync};
const SbEglInterface *SbGetEglInterface(void){return &interface;}
