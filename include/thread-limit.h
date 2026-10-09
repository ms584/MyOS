#ifndef MYOS_THREAD_LIMIT_H
#define MYOS_THREAD_LIMIT_H
/* Public YouTube VP9 selection exhausts32 workers before media startup.
 * Bounded40 workers + main;42 runtime TLS pages including guard fit48. */
#define MYOS_WORKER_LIMIT 40u
#define MYOS_THREAD_LIMIT (1u+MYOS_WORKER_LIMIT)
#define MYOS_PROCESS_LIMIT 4u
#define MYOS_USER_TASK_LIMIT (MYOS_PROCESS_LIMIT*MYOS_THREAD_LIMIT)
#endif
