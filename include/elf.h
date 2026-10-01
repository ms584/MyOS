#ifndef MYOS_ELF_H
#define MYOS_ELF_H
#include "os.h"
#define ELF_SEGMENTS 16u
#define ELF_SPAN (128ull*1024*1024)
#define ELF_BOOT_OFFSET 0x10010000ull
#define ELF_STACK_OFFSET 0x10001000ull
#define ELF_STACK_PAGES 4u
#define ELF_STARBOARD_STACK_PAGES 512u
#define ELF_TLS_LIMIT (60u*1024u)
struct elf_source { void *context; uint64_t size; bool (*read)(void *,uint64_t,void *,size_t); };
struct elf_segment { uint64_t offset,address,filesz,memsz; uint32_t flags; };
struct elf_plan { struct elf_segment segments[ELF_SEGMENTS]; unsigned count,stack_pages; uint64_t entry,dynamic_address,dynamic_size,relro_address,relro_size;
    uint64_t tls_address,tls_filesz,tls_size,tls_align,tls_offset; bool dynamic; };
int elf_plan_read(const struct elf_source *,struct elf_plan *);
int elf_inspect(const struct elf_source *);
struct process;
struct elf_start { uint64_t init,array,fini,fini_array,entry,preflight,handler,sabi;unsigned count,fini_count;const char *name,*url; };
bool elf_executable(const struct elf_plan *,uint64_t);
int elf_prepare(struct process *,const struct elf_plan *,struct elf_start *);
int elf_prepare_starboard(struct process *,const struct elf_plan *,struct elf_start *);
int elf_link(const struct elf_source *);
int elf_run(const struct elf_source *,int32_t *);
int elf_live(const struct elf_source *,int32_t *);
int elf_starboard(const struct elf_source *,const char *,const char *,int32_t *);
#endif
