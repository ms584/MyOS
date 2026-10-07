#ifndef MYOS_THREAD_LIMIT_H
#define MYOS_THREAD_LIMIT_H
/* Measured native Cobalt exhausts both8 and16 workers during startup.
 * Single-CPU bounded scheduler:32 workers plus one main per process. */
#define MYOS_WORKER_LIMIT 32u
#define MYOS_THREAD_LIMIT (1u+MYOS_WORKER_LIMIT)
#define MYOS_PROCESS_LIMIT 4u
#define MYOS_USER_TASK_LIMIT (MYOS_PROCESS_LIMIT*MYOS_THREAD_LIMIT)
#endif
