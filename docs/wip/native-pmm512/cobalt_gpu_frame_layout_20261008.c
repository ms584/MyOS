/* Host-only ABI layout for scheduler metadata and its saved interrupt frame. */
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
 printf("{\"task\":{\"size\":%zu,\"count\":%u,",sizeof(struct diagnostic_task),4+MYOS_USER_TASK_LIMIT);
 F(struct diagnostic_task,frame);F(struct diagnostic_task,process);F(struct diagnostic_task,thread);
 F(struct diagnostic_task,alive);F(struct diagnostic_task,blocked);
 printf("\"end\":0},\"thread\":{\"size\":%zu,",sizeof(struct process_thread));
 F(struct process_thread,tid);F(struct process_thread,cpu_nanos);
 printf("\"end\":0},\"frame\":{\"size\":%zu,",sizeof(struct frame));
 F(struct frame,rip);F(struct frame,cs);F(struct frame,vector);
 printf("\"end\":0}}\n");return 0;
}
