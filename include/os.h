#ifndef MYOS_OS_H
#define MYOS_OS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint64_t irq_save(void){uint64_t f;__asm__ volatile("pushfq; popq %0; cli":"=r"(f)::"memory");return f;}
static inline void irq_restore(uint64_t f){if(f&512)__asm__ volatile("sti":::"memory");}
void *memset(void *,int,size_t);
void *memcpy(void *,const void *,size_t);
void console_init(void);
void console_clear(void);
void putchar(char);
void print(const char *);
void hex(uint64_t);
void decimal(uint64_t);
void serial_put(char);
_Noreturn void panic(const char *);
void check(bool,const char *);
struct frame {
    uint64_t r15,r14,r13,r12,r11,r10,r9,r8,rdi,rsi,rbp,rdx,rcx,rbx,rax;
    uint64_t vector,error,rip,cs,flags,rsp,ss;
};
void interrupts_init(void);
void timer_init(void);
extern volatile uint64_t ticks;
void keyboard_init(void);
void keyboard_irq(void);
int keyboard_get(void);
void memory_init(void);
uintptr_t page_alloc(void);
void page_free(uintptr_t);
size_t pages_free(void);
bool vm_map(uintptr_t,uintptr_t,uint64_t);
uintptr_t vm_unmap(uintptr_t);
uintptr_t vm_translate(uintptr_t);
void *kmalloc(size_t);
void kfree(void *);
void memory_test(void);
void scheduler_init(void);
bool task_create(void (*)(void));
struct frame *schedule(struct frame *);
extern volatile uint64_t task_counts[2];
void scheduler_start(void);
_Noreturn void shell_run(void);
#define PAGE_SIZE 4096u
#endif
