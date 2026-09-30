#include <stddef.h>
#include <stdint.h>
#ifndef FINI_MODE
#define FINI_MODE 1
#endif
static volatile unsigned stage;
static void print(const char *text)
{
    size_t length=0;while(text[length])++length;
    __asm__ volatile("int $0x80"::"a"(1ull),"D"(text),"S"(length):"memory");
}
static void require(unsigned wanted)
{if(stage!=wanted){print("ELF finalizer ordering FAIL\n");__asm__ volatile("ud2");}}
void init_first(void){require(0);stage=1;}
static void constructor_one(void){require(1);stage=2;}
static void constructor_two(void){require(2);stage=3;}
uint64_t _start(void)
{
    require(3);stage=4;print("ELF finalizer entry returned\n");return 41;
}
static void destructor_one(void)
{
    require(5);stage=6;
#if FINI_MODE == 3
    print("ELF Ring 3 init/fini order and status PASS\n");
#endif
}
static void destructor_two(void){require(4);stage=5;}
void fini_last(void)
{
    require(FINI_MODE==2?4:6);stage=7;print("ELF Ring 3 init/fini order and status PASS\n");
}
__attribute__((section(".init_array"),used))
static void (*constructors[])(void)={constructor_one,constructor_two};
#if FINI_MODE != 2
__attribute__((section(".fini_array"),used))
static void (*destructors[])(void)={destructor_one,destructor_two};
#endif
