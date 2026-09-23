#ifndef MYOS_TIMER_H
#define MYOS_TIMER_H
/* Call after idt_init(), with interrupts disabled. Only IRQ0 is unmasked. */
void timer_init(void);
unsigned int timer_ticks(void);
#endif
