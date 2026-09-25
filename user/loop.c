/* Deliberately uncooperative user app for timeout/reaping acceptance. */
__attribute__((noreturn)) void _start(void)
{
    for(;;)__asm__ volatile("pause" ::: "memory");
}
