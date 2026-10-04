"""Sealed UTC converters/strftime and target calendar, not timezone/locale switching."""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
archive=root/'build/math-evaluation/musl-1.2.6.tar.gz'
with archive.open('rb') as stream:
    assert hashlib.file_digest(stream,'sha256').hexdigest()=='d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a'
base=root/'build/time-evaluation';base.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='sdk-',dir=base))
subprocess.run(['tar','-xzf',str(archive),'-C',str(out),
                *['musl-1.2.6/'+name for name in ('src/time/__secs_to_tm.c',
                  'src/time/__tm_to_secs.c','src/time/__year_to_secs.c','src/time/__month_to_secs.c',
                  'src/time/strftime.c','src/locale/langinfo.c','include/langinfo.h','COPYRIGHT')]],check=True)
original=out/'musl-1.2.6/src/time/__secs_to_tm.c'
content=original.read_text()
assert content.count('#include "time_impl.h"')==1 and content.count('__secs_to_tm')==1
source=out/'secs-to-tm.c'
source.write_text(content.replace('#include "time_impl.h"','#include <time.h>').replace(
    '__secs_to_tm','__myos_secs_to_tm'))
private=out/'myos-time-internal.h'
names=('ABDAY_1','DAY_1','ABMON_1','MON_1','D_T_FMT','D_FMT','T_FMT','T_FMT_AMPM','AM_STR','PM_STR')
lang=(out/'musl-1.2.6/include/langinfo.h').read_text()
macros=[]
for name in names:
    match=re.search(r'^#define '+name+r'\s+0x[0-9A-Fa-f]+$',lang,re.M);assert match,name
    macros.append(match.group(0))
private.write_text('#include <time.h>\ntypedef unsigned nl_item;\n'+ '\n'.join(macros)+'''\n
long long __myos_year_to_secs(long long,int *);
int __myos_month_to_secs(int,int);
long long __myos_tm_to_secs(const struct tm *);
const char *__myos_time_langinfo(nl_item);
size_t __myos_strftime(char *,size_t,const char *,const struct tm *);
static inline const char *__myos_tm_zone(const struct tm *t){return t->tm_zone?t->tm_zone:"";}
''')
extra_sources=[]
for name in ('__tm_to_secs','__year_to_secs','__month_to_secs'):
    text=(out/'musl-1.2.6/src/time'/(name+'.c')).read_text()
    text=text.replace('#include "time_impl.h"','#include "myos-time-internal.h"')
    # Widen before subtraction: INT_MIN day must not trigger signed-int UB.
    if name=='__tm_to_secs':
        assert '(tm->tm_mday-1)' in text
        text=text.replace('(tm->tm_mday-1)','((long long)tm->tm_mday-1)')
    for helper in ('__tm_to_secs','__year_to_secs','__month_to_secs'):
        text=text.replace(helper,'__myos'+helper[1:])
    path=out/(name+'.c');path.write_text(text);extra_sources.append((name,path))
original=(out/'musl-1.2.6/src/locale/langinfo.c').read_text()
start=original.index('static const char c_time[] =');end=original.index('\n\nstatic const char c_messages',start)
table=out/'time-langinfo.c';table.write_text('#include "myos-time-internal.h"\n'+original[start:end]+'''
const char *__myos_time_langinfo(nl_item item){
    if((item>>16)!=2 || (item&65535)>0x31)return "";
    const char *s=c_time;unsigned n=item&65535;
    for(;n;--n){while(*s)++s;++s;}return s;
}
''');extra_sources.append(('time-langinfo',table))
text=(out/'musl-1.2.6/src/time/strftime.c').read_text()
for inc in ('#include <langinfo.h>\n','#include <locale.h>\n','#include "locale_impl.h"\n'):
    assert text.count(inc)==1;text=text.replace(inc,'')
text=text.replace('#include "time_impl.h"','#include "myos-time-internal.h"')
for old,new in ((', locale_t loc',''),(', tm, loc',', tm'),
                ('__nl_langinfo_l(item, loc)','__myos_time_langinfo(item)'),
                ('tm->__tm_gmtoff','tm->tm_gmtoff'),('__tm_to_tzname','__myos_tm_zone'),
                ('__tm_to_secs','__myos_tm_to_secs'),('__strftime_l','__myos_strftime'),
                (', CURRENT_LOCALE','')):
    text=text.replace(old,new)
assert 'locale' not in text and 'loc,' not in text
text=text.split('\nweak_alias(')[0]
path=out/'strftime.c';path.write_text(text);extra_sources.append(('strftime',path))
include=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m64','-std=c11','-Os','-fPIC','-ffreestanding','-fno-builtin','-mno-red-zone',
       '-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables',
       '-nostdinc','-I'+str(root/'user/sdk'),'-isystem',include,
       '-D__myos__=1','-U__linux__','-U__linux','-Ulinux','-U__gnu_linux__',
       '-Wall','-Wextra','-Werror']
objects=[]
for name,path in [('calendar',root/'user/platform-calendar.c'),('convert',source),*extra_sources]:
    obj=out/(name+'.o')
    subprocess.run(['gcc',*flags,*(['-Wno-error=implicit-fallthrough','-Wno-error=sign-compare'] if name=='strftime' else []),
                    '-c',str(path),'-o',str(obj)],check=True)
    objects.append(obj)
library=out/'libmyos-calendar.a'
subprocess.run(['ar','rcs',str(library),*map(str,objects)],check=True)
print('UTC calendar SDK:',out,'converters/strftime + real format/scan/clock dependencies; '
      'NOT timezone database/DST or native execution.',flush=True)
