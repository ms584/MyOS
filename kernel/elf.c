#include "elf.h"
#include "process.h"
struct ehdr { uint8_t ident[16];uint16_t type,machine;uint32_t version;
    uint64_t entry,phoff,shoff;uint32_t flags;uint16_t ehsize,phentsize,phnum,shentsize,shnum,shstrndx; };
struct phdr { uint32_t type,flags;uint64_t offset,vaddr,paddr,filesz,memsz,align; };
struct shdr { uint32_t name,type;uint64_t flags,addr,offset,size;uint32_t link,info;uint64_t align,entsize; };
struct sym { uint32_t name;uint8_t info,other;uint16_t section;uint64_t value,size; };
_Static_assert(sizeof(struct ehdr)==64&&sizeof(struct phdr)==56&&sizeof(struct shdr)==64&&sizeof(struct sym)==24,"ELF layouts");
static bool read_at(const struct elf_source *s,uint64_t off,void *dst,size_t n)
{return off<=s->size&&n<=s->size-off&&s->read(s->context,off,dst,n);}
static bool header(const struct elf_source *s,struct ehdr *h)
{
    return s&&s->read&&read_at(s,0,h,sizeof(*h))&&h->ident[0]==127&&h->ident[1]=='E'&&
        h->ident[2]=='L'&&h->ident[3]=='F'&&h->ident[4]==2&&h->ident[5]==1&&h->ident[6]==1&&
        (h->type==2||h->type==3)&&h->machine==62&&h->version==1&&h->ehsize==64&&
        h->phentsize==56&&h->phnum&&h->phnum<=64&&h->phoff<=s->size&&
        (uint64_t)h->phnum*56<=s->size-h->phoff;
}
int elf_plan_read(const struct elf_source *s,struct elf_plan *plan)
{
    struct ehdr h;if(!plan||!header(s,&h))return -8;
    memset(plan,0,sizeof(*plan));plan->dynamic=h.type==3;
    uint64_t bias=plan->dynamic?USER_BASE:0;
    if(h.entry>UINT64_MAX-bias)return -8;
    plan->entry=h.entry+bias;bool entry=false;
    for(unsigned i=0;i<h.phnum;++i){
        struct phdr p;if(!read_at(s,h.phoff+i*56,&p,sizeof(p)))return -8;
        /* Interpreter and TLS require a runtime contract not supplied here. */
        if(p.type==3||p.type==7)return -95;
        if(p.type==2&&!plan->dynamic)return -95;
        if(p.type==0x6474e551&&(p.flags&1))return -8; /* executable stack */
        if(p.type==2||(p.type==0x6474e552&&plan->dynamic)){
            if(p.vaddr>ELF_SPAN||p.memsz>ELF_SPAN-p.vaddr||!plan->dynamic)return -8;
            if(p.type==2){if(plan->dynamic_size||!p.filesz||p.filesz>65536||p.filesz%16||p.filesz>p.memsz)return -8;
                plan->dynamic_address=p.vaddr+bias;plan->dynamic_size=p.filesz;}
            else{if(plan->relro_size)return -8;plan->relro_address=p.vaddr+bias;plan->relro_size=p.memsz;}
        }
        if(p.type!=1)continue;
        if(p.filesz>p.memsz||p.offset>s->size||p.filesz>s->size-p.offset||
           p.vaddr>UINT64_MAX-bias||(p.flags&~7u)||!(p.flags&4)||((p.flags&3)==3))return -8;
        if(p.align>1&&((p.align&(p.align-1))||p.vaddr%p.align!=p.offset%p.align))return -8;
        if(!p.memsz)continue;
        uint64_t address=p.vaddr+bias;
        if(address<USER_BASE||address>=USER_BASE+ELF_SPAN||p.memsz>USER_BASE+ELF_SPAN-address||plan->count==ELF_SEGMENTS)return -8;
        uint64_t lo=address&~4095ull,hi=(address+p.memsz+4095)&~4095ull;
        for(unsigned j=0;j<plan->count;++j){struct elf_segment *q=&plan->segments[j];
            uint64_t qlo=q->address&~4095ull,qhi=(q->address+q->memsz+4095)&~4095ull;
            if(lo<qhi&&qlo<hi)return -8;
        }
        plan->segments[plan->count++]=(struct elf_segment){p.offset,address,p.filesz,p.memsz,p.flags};
        if((p.flags&1)&&plan->entry>=address&&plan->entry-address<p.filesz)entry=true;
    }
    if(!plan->count||(!plan->dynamic&&!entry))return -8;
    return 0;
}
int elf_inspect(const struct elf_source *s)
{
    struct elf_plan plan;int result=elf_plan_read(s,&plan);if(result)return result;
    struct ehdr h;if(!header(s,&h))return -8;
    print("ELF64: ");print(plan.dynamic?"ET_DYN":"ET_EXEC");print(" file=");decimal(s->size);
    print(" segments=");decimal(plan.count);print("\n");
    uint64_t bytes=0;for(unsigned i=0;i<plan.count;++i)bytes+=plan.segments[i].memsz;
    print("ELF LOAD memory bytes: ");decimal(bytes);print("\n");
    if(!h.shnum){print("ELF: no section import inventory\n");return 0;}
    if(h.shnum>4096||h.shentsize!=64||h.shoff>s->size||(uint64_t)h.shnum*64>s->size-h.shoff)return -8;
    unsigned strong=0,weak=0;
    for(unsigned i=0;i<h.shnum;++i){
        struct shdr sec;if(!read_at(s,h.shoff+i*64,&sec,sizeof(sec)))return -8;
        if(sec.type!=11)continue;
        if(sec.entsize!=24||sec.size%24||sec.size/24>65536||sec.offset>s->size||sec.size>s->size-sec.offset||sec.link>=h.shnum)return -8;
        struct shdr strings;if(!read_at(s,h.shoff+sec.link*64,&strings,sizeof(strings))||strings.type!=3||strings.offset>s->size||strings.size>s->size-strings.offset)return -8;
        for(uint64_t pos=0;pos<sec.size;pos+=24){
            struct sym sym;if(!read_at(s,sec.offset+pos,&sym,sizeof(sym)))return -8;
            if(sym.section||!sym.name)continue;
            unsigned bind=sym.info>>4;if(bind!=1&&bind!=2)return -8;
            if(sym.name>=strings.size)return -8;
            char name[256];size_t n=0;
            do{if(n==sizeof(name)-1||sym.name+n>=strings.size||!read_at(s,strings.offset+sym.name+n,name+n,1))return -8;
                unsigned char c=(unsigned char)name[n];if(c&&(c<33||c>126))return -8;
            }while(name[n++]);
            print(bind==2?"ELF weak: ":"ELF unresolved: ");print(name);print("\n");
            if(bind==2)++weak;else ++strong;
        }
    }
    print("ELF imports: strong=");decimal(strong);print(" weak=");decimal(weak);print("\n");
    if(plan.dynamic)print("ELF: inspection only; not executed\n");
    return 0;
}
static int load_image(const struct elf_source *s,int32_t *status,bool run,bool live)
{
    struct elf_plan plan;int result=elf_plan_read(s,&plan);if(result)return result;
    /* A shared library with no executable entry can be linked/inspected only. */
    if(run&&!elf_executable(&plan,plan.entry))return -95;
    size_t before=pages_free(),heap_before=heap_allocated();
    struct process *p=process_stage_elf(s,&plan);
    if(!p){check(pages_free()==before,"ELF failed-load page reclamation");
        if(live)check(heap_allocated()==heap_before,"ELF live failed-load heap reclamation");return -12;}
    print("ELF staged pages: ");decimal(p->page_count);print("\n");
    struct elf_start start;result=elf_prepare(p,&plan,&start);
    if(result||!run){process_discard_elf(p);
        check(pages_free()==before,"ELF staged page reclamation");
        if(live)check(heap_allocated()==heap_before,"ELF live staged heap reclamation");
        print("ELF staged image released; not executed\n");return result;}
    uint64_t pid=p->pid;
    if(!process_start_elf(p,start.init,start.array,start.count)){
        process_discard_elf(p);check(pages_free()==before,"ELF attach cleanup");
        if(live)check(heap_allocated()==heap_before,"ELF live attach heap reclamation");return -12;}
    /* Explicit foreground lifecycle opt-in; fixture run retains its 5s limit.
     * No application-thread cooperation is needed for forced cancellation. */
    bool cancelled=false;uint64_t deadline=live?UINT64_MAX:ticks+500;
    if(live)print("ELF live: started; Escape stops all threads\n");
    while(p->state==PROCESS_RUNNING&&ticks<deadline){
        if(live){int key;while((key=keyboard_get())>=0)if(key==27){cancelled=true;break;}
            if(cancelled)break;}
        __asm__ volatile("sti; hlt":::"memory");
    }
    if(p->state==PROCESS_RUNNING){check(process_kill(pid),"ELF cancellation stop");result=cancelled?-125:-110;}
    else if(p->exited){*status=p->exit_status;result=0;}
    else result=-8;
    check(process_reap(pid),"ELF process reap");
    check(pages_free()==before,"ELF page reclamation");
    if(live){
        /* Applications may legitimately grow persistent RAMFS state. Report
         * the global heap count; controlled fixtures assert a zero delta. */
        print("ELF live: pages reclaimed; heap-before=");decimal(heap_before);
        print(" heap-after=");decimal(heap_allocated());print("\n");}
    return result;
}
int elf_run(const struct elf_source *s,int32_t *status){return load_image(s,status,true,false);}
int elf_live(const struct elf_source *s,int32_t *status){return load_image(s,status,true,true);}
int elf_link(const struct elf_source *s){return load_image(s,0,false,false);}
