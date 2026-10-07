/* Read-only diagnostic layout, compiled against the actual private headers. */
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
 F(struct diagnostic_task,frame);F(struct diagnostic_task,alive);F(struct diagnostic_task,process);
 F(struct diagnostic_task,thread);F(struct diagnostic_task,blocked);F(struct diagnostic_task,wait_address);F(struct diagnostic_task,wait_deadline);
 printf("\"end\":0},\"process\":{\"size\":%zu,\"count\":%u,",sizeof(struct process),PROCESS_LIMIT);
 F(struct process,pid);F(struct process,state);F(struct process,root);F(struct process,vm_page_count);F(struct process,vm_areas);
 printf("\"end\":0},\"thread\":{");F(struct process_thread,tid);
 printf("\"end\":0},\"frame\":{\"size\":%zu,",sizeof(struct frame));
 F(struct frame,rip);F(struct frame,rsp);F(struct frame,rbp);F(struct frame,rdi);F(struct frame,rsi);F(struct frame,rdx);F(struct frame,rax);
 printf("\"end\":0}}\n");return 0;
}
