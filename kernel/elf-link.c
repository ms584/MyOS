#include "elf.h"
#include "process.h"
#include "platform-runtime-exports.h"
/* Single-object eager linker with an exact allowlist of tested platform exports. */
struct dyn { int64_t tag;uint64_t value; };
struct symbol { uint32_t name;uint8_t info,other;uint16_t section;uint64_t value,size; };
struct rela { uint64_t offset,info;int64_t addend; };
struct linker {
    struct process *process;const struct elf_plan *plan;
    uint64_t dt[34],seen,gnu_hash,gnu_bytes,hash_bytes,flags1,relacount;
    unsigned symbols,missing,weak,platform,relative,absolute,global,jump,skipped;
};
struct platform_export { const char *name;uint64_t address; };
#define PLATFORM_NAME(symbol,address) static char platform_name_##symbol[] \
    __attribute__((section(".platform_runtime_names")))=#symbol;
PLATFORM_EXPORTS(PLATFORM_NAME)
#undef PLATFORM_NAME
#define PLATFORM_ITEM(symbol,address) {platform_name_##symbol,address},
static struct platform_export platform_exports[]
    __attribute__((section(".platform_runtime_names")))={
    PLATFORM_EXPORTS(PLATFORM_ITEM)
};
#undef PLATFORM_ITEM
static bool name_equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool platform_resolve(const char *name,uint64_t *address)
{
    for(unsigned i=0;i<sizeof(platform_exports)/sizeof(platform_exports[0]);++i)
        if(name_equal(name,platform_exports[i].name)){*address=platform_exports[i].address;return true;}
    return false;
}
static bool span(const struct elf_plan *plan,uint64_t offset,uint64_t size,unsigned flags)
{
    if(offset>=ELF_SPAN||size>ELF_SPAN-offset)return false;
    for(unsigned i=0;i<plan->count;++i){const struct elf_segment *s=&plan->segments[i];
        uint64_t lo=s->address-USER_BASE;
        if(offset>=lo&&offset-lo<=s->memsz&&size<=s->memsz-(offset-lo)&&(s->flags&flags)==flags)return true;
    }return false;
}
static bool read_image(struct linker *l,uint64_t offset,void *dst,size_t size)
{return span(l->plan,offset,size,4)&&process_image_copy(l->process,USER_BASE+offset,dst,size,false);}
static bool relro_pages(const struct elf_plan *plan,uint64_t address,uint64_t size)
{
    /* GNU ld may include final-page padding beyond p_memsz in PT_GNU_RELRO.
     * Only accept pages actually mapped by ONE writable, nonexecuting LOAD. */
    if(address<USER_BASE||address>=USER_BASE+ELF_SPAN||size>USER_BASE+ELF_SPAN-address)return false;
    for(unsigned i=0;i<plan->count;++i){const struct elf_segment *s=&plan->segments[i];
        uint64_t end=s->address+s->memsz;
        if(s->flags==6&&address>=s->address&&address<end&&
           address+size<=((end+PAGE_SIZE-1)&~(PAGE_SIZE-1ull)))return true;
    }return false;
}
static bool overlap(uint64_t a,uint64_t n,uint64_t b,uint64_t m)
{return n&&m&&a<b+m&&b<a+n;}
static bool target(struct linker *l,uint64_t offset,unsigned width)
{
    if(!span(l->plan,offset,width,6))return false;
    uint64_t tables[][2]={{l->plan->dynamic_address-USER_BASE,l->plan->dynamic_size},
        {l->dt[7],l->dt[8]},{l->dt[23],l->dt[2]},
        {l->dt[6],(uint64_t)l->symbols*24},{l->dt[5],l->dt[10]},
        {l->gnu_hash,l->gnu_bytes},{l->dt[4],l->hash_bytes}};
    for(unsigned i=0;i<sizeof(tables)/sizeof(tables[0]);++i)
        if(overlap(offset,width,tables[i][0],tables[i][1]))return false;
    return true;
}
static int parse(struct linker *l)
{
    if(!l->plan->dynamic_size)return -8;
    bool end=false,gnu=false,flags=false,count=false;
    for(uint64_t pos=0;pos<l->plan->dynamic_size;pos+=16){
        struct dyn d;if(!read_image(l,l->plan->dynamic_address-USER_BASE+pos,&d,sizeof(d)))return -8;
        if(!d.tag){end=true;break;}
        if(d.tag==0x6ffffef5){if(gnu)return -8;gnu=true;l->gnu_hash=d.value;continue;}
        if(d.tag==0x6ffffffb){if(flags)return -8;flags=true;l->flags1=d.value;if(d.value&~0x8000001ull)return -95;continue;}
        if(d.tag==0x6ffffff9){if(count)return -8;count=true;l->relacount=d.value;continue;}
        if(d.tag<1||d.tag>33)return -95;
        if(l->seen&(1ull<<d.tag))return -8;
        l->seen|=1ull<<d.tag;l->dt[d.tag]=d.value;
        switch(d.tag){
        case 1:case 17:case 18:case 19:case 32:case 33:return -95;
        case 22:return -8;
        case 30:if(d.value&~26ull)return -95;break;
        default:break;
        }
    }
    if(!end||(l->dt[8]&&l->dt[9]!=24)||l->dt[11]!=24||!l->dt[5]||!l->dt[6]||!l->dt[10]||
       l->dt[8]%24||l->dt[2]%24||l->dt[8]/24>200000||l->dt[2]/24>65536||
       (l->dt[2]&&l->dt[20]!=7)||l->relacount>l->dt[8]/24)return -8;
    if(!span(l->plan,l->dt[5],l->dt[10],4)||
       (l->dt[8]&&!span(l->plan,l->dt[7],l->dt[8],4))||
       (l->dt[2]&&!span(l->plan,l->dt[23],l->dt[2],4))||
       overlap(l->dt[7],l->dt[8],l->dt[23],l->dt[2]))return -8;
    unsigned sysv_count=0,gnu_count=0;
    if(l->dt[4]){uint32_t h[2];if(!read_image(l,l->dt[4],h,sizeof(h))||!h[0]||h[0]>65536||!h[1]||h[1]>65536)return -8;
        l->hash_bytes=8+4ull*(h[0]+h[1]);if(!span(l->plan,l->dt[4],l->hash_bytes,4))return -8;sysv_count=h[1];}
    if(l->gnu_hash){uint32_t h[4];if(!read_image(l,l->gnu_hash,h,sizeof(h))||!h[0]||h[0]>65536||h[1]>65536||!h[2]||h[2]>65536||(h[2]&(h[2]-1))||h[3]>=64)return -8;
        uint64_t buckets=l->gnu_hash+16+8ull*h[2],chains=buckets+4ull*h[0];uint32_t last=0;
        if(!span(l->plan,l->gnu_hash,chains-l->gnu_hash,4))return -8;
        for(unsigned i=0;i<h[0];++i){uint32_t value;if(!read_image(l,buckets+4ull*i,&value,4)||
            (value&&(value<h[1]||value>=65536)))return -8;if(value>last)last=value;}
        gnu_count=h[1];
        if(last){for(unsigned i=last;i<65536;++i){uint32_t value;
            if(!read_image(l,chains+4ull*(i-h[1]),&value,4))return -8;
            if(value&1){gnu_count=i+1;break;}}
            if(gnu_count<=last)return -8;}
        l->gnu_bytes=chains-l->gnu_hash+4ull*(gnu_count-h[1]);
        /* Empty GNU buckets omit unhashable undefined symbols. Only the
         * independently bounded SysV table can give their full count. */
        if(!last&&sysv_count>=gnu_count)gnu_count=sysv_count;
    }
    if(!sysv_count&&!gnu_count)return -8;
    if(sysv_count&&gnu_count&&sysv_count!=gnu_count)return -8;
    l->symbols=sysv_count?sysv_count:gnu_count;
    if(!span(l->plan,l->dt[6],24ull*l->symbols,4))return -8;
    return 0;
}
static int get_symbol(struct linker *l,unsigned index,struct symbol *s)
{
    if(index>=l->symbols||!read_image(l,l->dt[6]+24ull*index,s,sizeof(*s)))return -8;
    if((s->info&15)==10||s->info>>4>2)return -95; /* IFUNC/unique */
    if(s->name>=l->dt[10])return -8;
    if((s->info&15)==6){
        if(!l->plan->tls_size||!s->section)return -95; /* No external TLS module. */
        if(s->section>=0xff00||s->value>=l->plan->tls_size||s->size>l->plan->tls_size-s->value)return -8;
    }
    return 0;
}
static int symbol_name(struct linker *l,const struct symbol *s,char name[256])
{
    if(!s->name||s->name>=l->dt[10])return -8;
    unsigned n=0;do{
        if(n==255||s->name+n>=l->dt[10]||!read_image(l,l->dt[5]+s->name+n,name+n,1))return -8;
        unsigned char c=(unsigned char)name[n];if(c&&(c<33||c>126))return -8;
    }while(name[n++]);return 0;
}
static int imports(struct linker *l)
{
    for(unsigned i=1;i<l->symbols;++i){struct symbol s;int rc=get_symbol(l,i,&s);if(rc)return rc;
        if(s.section)continue;
        if(!s.name||!(s.info>>4))return -8;
        char name[256];if(symbol_name(l,&s,name))return -8;uint64_t address;
        if(platform_resolve(name,&address)){++l->platform;print("ELF platform: ");print(name);print("\n");continue;}
        if((s.info>>4)==2){++l->weak;print("ELF weak zero: ");}
        else{++l->missing;print("ELF missing: ");}
        print(name);print("\n");
    }return 0;
}
static int value(struct linker *l,unsigned index,uint64_t *out,bool *missing)
{
    struct symbol s;int rc=get_symbol(l,index,&s);if(rc)return rc;*missing=false;
    if((s.info&15)==6)return -8; /* TLS offsets are not ordinary image VAs. */
    if(!s.section){
        char name[256];if(index&&symbol_name(l,&s,name))return -8;
        if(index&&platform_resolve(name,out))return 0;
        *out=0;*missing=index&&(s.info>>4)!=2;return 0;
    }
    if(s.section==0xfff1){*out=s.value;return 0;}
    if(s.section>=0xff00||!span(l->plan,s.value,s.size,4))return -8;
    *out=USER_BASE+s.value;return 0;
}
static bool add_signed(uint64_t base,int64_t add,uint64_t *out)
{
    if(add>=0){if((uint64_t)add>UINT64_MAX-base)return false;*out=base+(uint64_t)add;}
    else{uint64_t magnitude=0-(uint64_t)add;if(base<magnitude)return false;*out=base-magnitude;}
    return true;
}
static int relocate(struct linker *l,uint64_t start,uint64_t size)
{
    for(uint64_t i=0;i<size;i+=24){struct rela r;if(!read_image(l,start+i,&r,sizeof(r)))return -8;
        unsigned type=(uint32_t)r.info,index=(unsigned)(r.info>>32);uint64_t resolved=0;bool missing=false;
        if(!type)continue;
        if(!target(l,r.offset,type==36?16:8))return -8;
        if(type==8){if(index||!add_signed(USER_BASE,r.addend,&resolved)||resolved<USER_BASE||resolved>USER_BASE+ELF_SPAN)return -8;++l->relative;}
        else if(type==1||type==6||type==7){int rc=value(l,index,&resolved,&missing);if(rc)return rc;
            if(type==1){if(!missing&&!add_signed(resolved,r.addend,&resolved))return -8;++l->absolute;}
            else{if(r.addend)return -8;if(type==6)++l->global;else ++l->jump;}
        }else if(type==16||type==17||type==18||type==36){
            if(!l->plan->tls_size)return -95;
            uint64_t offset=0;
            if(index){struct symbol s;int rc=get_symbol(l,index,&s);if(rc)return rc;
                if((s.info&15)!=6)return -8;offset=s.value;}
            if(type==16){if(r.addend)return -8;resolved=1;}
            else {
                if(!add_signed(offset,r.addend,&offset)||offset>=l->plan->tls_size)return -8;
                resolved=type==17?offset:offset-l->plan->tls_offset;
            }
            if(type==36){uint64_t address;if(!platform_resolve("MyOSTlsDesc",&address))return -95;
                uint64_t descriptor[]={address,resolved};
                if(!process_image_copy(l->process,USER_BASE+r.offset,descriptor,16,true))return -8;
                continue;
            }
        }else return -95;
        if(missing){++l->skipped;continue;}
        if(!process_image_copy(l->process,USER_BASE+r.offset,&resolved,8,true))return -8;
    }return 0;
}
bool elf_executable(const struct elf_plan *plan,uint64_t address)
{
    for(unsigned i=0;i<plan->count;++i){const struct elf_segment *s=&plan->segments[i];
        if((s->flags&1)&&address>=s->address&&address-s->address<s->filesz)return true;
    }return false;
}
/* Compare only the bounded requested name, not every potentially long C++
 * export. Entry points must be unambiguous strong functions wholly in RX data. */
static int exported_function(struct linker *l,const char *name,uint64_t *address)
{
    bool found=false;
    for(unsigned i=1;i<l->symbols;++i){struct symbol s;int rc=get_symbol(l,i,&s);if(rc)return rc;
        if(!s.name)continue;bool matches=true;
        for(unsigned n=0;;++n){char c;
            if(s.name+n>=l->dt[10]||!read_image(l,l->dt[5]+s.name+n,&c,1))return -8;
            if(c!=name[n]){matches=false;break;}if(!c)break;
        }
        if(!matches)continue;
        if(found||!s.section||s.section>=0xff00||s.info!=0x12||
           (s.other!=0&&s.other!=3)||!s.size||!span(l->plan,s.value,s.size,5))return -8;
        bool backed=false;
        for(unsigned j=0;j<l->plan->count;++j){const struct elf_segment *segment=&l->plan->segments[j];
            uint64_t lo=segment->address-USER_BASE;
            if((segment->flags&5)==5&&s.value>=lo&&s.value-lo<segment->filesz&&
               s.size<=segment->filesz-(s.value-lo)){backed=true;break;}}
        if(!backed)return -8;
        *address=USER_BASE+s.value;found=true;
    }
    return found?0:-95;
}
static int prepare(struct process *process,const struct elf_plan *plan,struct elf_start *start,bool starboard)
{
    memset(start,0,sizeof(*start));if(!plan->dynamic)return 0;
    struct linker l={.process=process,.plan=plan};int rc=parse(&l);if(rc)return rc;
    if((rc=imports(&l)))return rc;
    if((l.platform||starboard||plan->tls_size)&&!process_map_runtime(process))return -12;
    if((rc=relocate(&l,l.dt[7],l.dt[8]))||(rc=relocate(&l,l.dt[23],l.dt[2])))return rc;
    print("ELF relocations: relative=");decimal(l.relative);print(" absolute=");decimal(l.absolute);
    print(" global=");decimal(l.global);print(" jump=");decimal(l.jump);print(" skipped=");decimal(l.skipped);print("\n");
    print("ELF bindings: missing=");decimal(l.missing);print(" weak-zero=");decimal(l.weak);print("\n");
    if(plan->relro_size){
        if(!relro_pages(plan,plan->relro_address,plan->relro_size)||
           (plan->relro_address+plan->relro_size)%PAGE_SIZE||
           !process_image_readonly(process,plan->relro_address,plan->relro_size))return -8;
        print("ELF RELRO: protected\n");
    }
    if(l.missing)return -95; /* Never call constructors/entry against stubs. */
    if(!process_install_tls(process,plan))return -12;
    if(starboard){
        if((rc=exported_function(&l,"SbEventHandle",&start->handler))||
           (rc=exported_function(&l,"GetEvergreenSabiString",&start->sabi)))return rc;
        if(!platform_resolve("MyOSStarboardRun",&start->entry)||
           !platform_resolve("MyOSStarboardValidate",&start->preflight))return -95;
    }
    if(l.dt[13]){if(l.dt[13]>=ELF_SPAN)return -8;
        start->fini=USER_BASE+l.dt[13];if(!elf_executable(plan,start->fini))return -8;}
    if(l.dt[28]%8||l.dt[28]>128*8)return -8;
    if(l.dt[28]){
        if(!span(plan,l.dt[26],l.dt[28],4))return -8;
        start->fini_array=USER_BASE+l.dt[26];start->fini_count=(unsigned)(l.dt[28]/8);
        for(unsigned i=0;i<start->fini_count;++i){uint64_t address;
            if(!read_image(&l,l.dt[26]+8ull*i,&address,8)||!elf_executable(plan,address))return -8;
        }
    }
    if(l.dt[12]){if(l.dt[12]>=ELF_SPAN)return -8;
        start->init=USER_BASE+l.dt[12];if(!elf_executable(plan,start->init))return -8;}
    if(l.dt[27]%8||l.dt[27]>128*8)return -8;
    if(l.dt[27]){
        if(!span(plan,l.dt[25],l.dt[27],4))return -8;
        start->array=USER_BASE+l.dt[25];start->count=(unsigned)(l.dt[27]/8);
        for(unsigned i=0;i<start->count;++i){uint64_t address;
            if(!read_image(&l,l.dt[25]+8ull*i,&address,8)||!elf_executable(plan,address))return -8;
        }
    }
    return 0;
}
int elf_prepare(struct process *p,const struct elf_plan *plan,struct elf_start *start)
{return prepare(p,plan,start,false);}
int elf_prepare_starboard(struct process *p,const struct elf_plan *plan,struct elf_start *start)
{return prepare(p,plan,start,true);}
