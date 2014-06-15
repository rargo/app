#ifndef TIMER_H
#define TIMER_H

#define SYS_TIMER_PRIO 2

void sys_timer_init(unsigned int interval_ms);
void sys_timer_start(void);
void sys_timer_stop(void);
int sys_timer_attach_handler(event_handler_t *h);

#endif
