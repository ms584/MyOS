/* Host-only metadata layout, compiled against the actual private headers. */
#include <stdio.h>
#include <stddef.h>
#define putchar myos_kernel_putchar
#include <process.h>
#undef putchar
struct diagnostic_task {
 struct frame *frame;void (*entry)(void);void *stack;bool alive;
 struct process *process;struct process_thread *thread;bool blocked;
 uint64_t wait_address,wait_deadline,fs_base;
};
#define F(type,member) printf("\"" #member "\":%zu,",offsetof(type,member))
int main(void){
 printf("{\"process\":{\"size\":%zu,\"count\":%u,",sizeof(struct process),PROCESS_LIMIT);
 F(struct process,pid);F(struct process,state);F(struct process,cpu_nanos);
 F(struct process,main_cpu_nanos);F(struct process,preemptions);F(struct process,threads);
 printf("\"end\":0},\"thread\":{\"size\":%zu,\"count\":%u,",sizeof(struct process_thread),PROCESS_THREAD_LIMIT);
 F(struct process_thread,tid);F(struct process_thread,state);F(struct process_thread,cpu_nanos);
 printf("\"end\":0},\"task\":{\"size\":%zu,\"count\":%u,",sizeof(struct diagnostic_task),4+MYOS_USER_TASK_LIMIT);
 F(struct diagnostic_task,alive);F(struct diagnostic_task,process);F(struct diagnostic_task,thread);
 F(struct diagnostic_task,blocked);F(struct diagnostic_task,wait_deadline);
 printf("\"end\":0}}\n");return 0;
}
