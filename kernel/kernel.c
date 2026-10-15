#include "app.h"
#include "syscall.h"
#include "storage.h"
#include "graphics.h"
#include "mouse.h"
#include "usb.h"
#include "settings.h"
#include "snake.h"
#include "nic.h"
#include "audio.h"
#include "security.h"
#ifndef TEST_CASE
#define TEST_CASE 0
#endif
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
#endif
extern _Noreturn void context_probe(volatile uint64_t *);
static void worker_a(void){context_probe(&task_counts[0]);}
static void worker_b(void){context_probe(&task_counts[1]);}
static volatile bool returned_task;
static void once(void){returned_task=true;}
void kernel_main(void){
    console_init();print("MyOS Kernel: 64-bit Long Mode\n");
    interrupts_init();print("IDT/PIC initialized\n");
    memory_init();memory_test();gfx_test();
    print("Free physical pages: ");decimal(pages_free());print("\n");
    fs_init();fs_test();
    process_init();print("GDT/TSS and Ring 3 selectors initialized\n");
    keyboard_init();print("PS/2 keyboard initialized\n");
    struct gfx_mode mouse_mode;bool video=gfx_probe(&mouse_mode);
    if(mouse_init(video?mouse_mode.width:640,video?mouse_mode.height:400))
        print("PS/2 mouse initialized\n");
    else print("PS/2 mouse unavailable\n");
    scheduler_init();check(task_create(worker_a)&&task_create(worker_b)&&task_create(once),"task creation");
    check(!task_create(once),"task capacity");
    timer_init();scheduler_start();print("PIT: 100 Hz; preemptive scheduler started\n");
    __asm__ volatile("sti":::"memory");
    while(ticks<100)__asm__ volatile("hlt");
    check(task_counts[0]&&task_counts[1]&&returned_task,"preemption/task exit");
    uint64_t a=task_counts[0],b=task_counts[1],target=ticks+100;
    while(ticks<target)__asm__ volatile("hlt");
    check(task_counts[0]>a&&task_counts[1]>b,"sustained preemption");
    print("PASS: two CPU-bound tasks preempted; task exit\n");
    print("PASS: context registers and direction flag preserved\n");
    process_test();
    syscall_test();
    fs_user_test();
#if !PROCESS_TESTS
    scheduler_finish_boot_tests();
    print("PASS: boot kernel probes retired\n");
#endif
#if TEST_CASE == 1
    __asm__ volatile(".global fault_site\nfault_site: ud2");
#elif TEST_CASE == 2
    *(volatile uint64_t *)0x200000000ull=1;
#elif TEST_CASE == 3
    uintptr_t page=page_alloc();check(vm_map(0x200000000ull,page,1ull<<63),"RO map");
    *(volatile uint64_t *)0x200000000ull=1;
#elif TEST_CASE == 4
    uintptr_t page=page_alloc();check(vm_map(0x200000000ull,page,2|(1ull<<63)),"NX map");
    *(volatile uint8_t *)page=0xc3;((void (*)(void))0x200000000ull)();
#elif TEST_CASE == 5
    print("PASS: automated boot\n");outb(0xf4,0x10);
#elif TEST_CASE == 6
    __asm__ volatile("mov $0x48, %%ax\n.global fault_site\nfault_site: mov %%ax, %%ds":::"rax");
#endif
    usb_init();
    storage_init();
    int mount=storage_load();
    if(!mount){print("Storage: restored RAMFS generation ");decimal(storage_generation());print("\n");}
    else if(mount==-2 && storage_ready())print("Storage: empty data disk; first change will save\n");
    else if(mount!=-19){print("Storage: snapshot unavailable (error ");decimal((uint64_t)(-mount));print("); RAM contents retained\n");}
    if(nic_init())print("NIC: RTL8139 initialized\n");
    if(audio_init())print("Audio: SB16 initialized\n");
    security_init();security_status();
    app_seed();
    settings_init();
    snake_init();
    shell_run();
}
