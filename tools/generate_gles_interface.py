"""Generate exact pinned GLES table; compiler checks every real entry point.

Generated build artifacts only. No name-to-success stub or host GLES provider.
"""
import re
from pathlib import Path

def generate(header: Path, output: Path) -> Path:
    block=header.read_text().split('typedef struct SbGlesInterface {',1)[1].split('} SbGlesInterface;',1)[0]
    block=re.sub(r'//[^\n]*|/\*.*?\*/','',block,flags=re.S)
    names=re.findall(r'\(\*(gl[A-Za-z0-9_]+)\)\s*\(',block)
    assert names and len(names)==len(set(names)) and block.count('(*')==len(names)
    # Header layout and each typed assignment are checked by the target compiler.
    text=['/* Generated from hash-verified pinned Starboard GLES declaration. */',
          '#define GL_GLEXT_PROTOTYPES','#include <stddef.h>',
          '#include <GL/gl.h>','#include <GL/glext.h>','#include <EGL/egl.h>',
          '#include "c11/threads.h"','#include "starboard/gles.h"',
          'const SbGlesInterface *SbGetGlesInterface(void);',
          f'_Static_assert(sizeof(SbGlesInterface)=={len(names)}*sizeof(void (*)(void)),"GLES table size");',
          '_Static_assert(sizeof(SbGlIntPtr)==sizeof(GLintptr),"GLES pointer integer ABI");',
          '_Static_assert(sizeof(SbGlSizeiPtr)==sizeof(GLsizeiptr),"GLES size ABI");']
    text.extend(f'_Static_assert(offsetof(SbGlesInterface,{name})=={i}*sizeof(void (*)(void)),"GLES {name} offset");'
                for i,name in enumerate(names))
    # This actual Mesa GLES3 procedure exists in its dispatch XML/implementation
    # but is not exported as a standalone symbol in the pinned glapi archive.
    # Resolve only that entry through Mesa's genuine procedure lookup, once.
    indirect='glGetInternalformativ';assert indirect in names
    text.append('static SbGlesInterface interface={')
    text.extend(f'    .{name}={"NULL" if name==indirect else name},' for name in names)
    text.extend(['};',
        '_Static_assert(__builtin_types_compatible_p(__typeof__(interface.glGetInternalformativ),'
        '__typeof__(&glGetInternalformativ)),"internal format query ABI");',
        'static once_flag once=ONCE_FLAG_INIT;',
        'static void initialize(void){interface.glGetInternalformativ='
        '(__typeof__(interface.glGetInternalformativ))eglGetProcAddress("glGetInternalformativ");}',
        'const SbGlesInterface *SbGetGlesInterface(void){call_once(&once,initialize);'
        'return interface.glGetInternalformativ?&interface:NULL;}'])
    source=output/'gles-interface.c';source.write_text('\n'.join(text)+'\n')
    dispatch=['/* Native fixture calls the pinned interface, not a parallel GL path. */',
              '#include "starboard/gles.h"','const SbGlesInterface *SbGetGlesInterface(void);',
              f'#define MYOS_GLES_INTERFACE_SLOTS {len(names)}']
    dispatch.extend(f'#define {name} (SbGetGlesInterface()->{name})' for name in names)
    (output/'include/gles-dispatch.h').write_text('\n'.join(dispatch)+'\n')
    return source
