#include "syscall.h"
#ifndef PROCESS_TESTS
#define PROCESS_TESTS 0
#endif
static struct process *wait_process(uint64_t pid,bool running)
{
    struct process *p=process_find(pid);check(p!=0,"filesystem user process");
    uint64_t deadline=ticks+400;
    while(ticks<deadline){
        if(running?(*(volatile uint64_t *)p->data_page!=0):(p->state==PROCESS_ZOMBIE))return p;
        __asm__ volatile("hlt":::"memory");
    }
    panic("filesystem user process timeout");
}
void fs_user_test(void)
{
    size_t before=pages_free();unsigned nodes=fs_node_count();
#if PROCESS_TESTS
    uint64_t pid=process_create(17,0);struct process *p=wait_process(pid,false);
    check(p->exited&&p->exit_status==0&&((uint64_t *)p->data_page)[7]==18,"file syscall contract");
    check(!fs_unlink("/home/user.txt"),"exit closes descriptors before reap");
    check(process_reap(pid)&&pages_free()==before,"filesystem exit reclamation");
    pid=process_create(19,0);p=wait_process(pid,false);
    check(!p->exited&&p->fault_vector==6&&!fs_unlink("/home/user.txt"),"fault closes descriptors");
    check(process_reap(pid)&&pages_free()==before,"filesystem fault reclamation");
    pid=process_create(20,0);p=wait_process(pid,true);
    check(p->state==PROCESS_RUNNING&&process_kill(pid)&&!fs_unlink("/home/user.txt"),"kill closes descriptors");
    check(process_reap(pid)&&pages_free()==before,"filesystem kill reclamation");
    print("PASS: file syscalls, safe copyout, exit/fault/kill descriptor cleanup\n");
#endif
    uint64_t demo=process_create(18,0);struct process *result=wait_process(demo,false);
    check(result->exited&&result->exit_status==0,"user reads motd");
    check(process_reap(demo)&&pages_free()==before&&fs_node_count()==nodes,"filesystem demo reclaimed");
    print("PASS: Ring 3 opened/read/closed /etc/motd\n");
}
