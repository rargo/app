#include <base.h>
#include <timer.h>
#include <irq.h>
#include <ucos.h>



#define MAX_SYS_TIMER_HANDLER 3
static event_handler_t sys_timer_handler[MAX_SYS_TIMER_HANDLER];


/* careful timer handler should as quick as possible */
void sys_timer_irq(void)
{
	int i;

	for(i = 0; i < MAX_SYS_TIMER_HANDLER; i++) {
		if(sys_timer_handler[i].fn == NULL)
			continue;
		sys_timer_handler[i].fn(sys_timer_handler[i].arg);
	}
	TINT_CSTAT_REG |= 1<<9; 
}

/* timer4 is use as system timer */
void sys_timer_init(unsigned int interval_ms)
{
	TCON_REG &= ~(1<<20);//stop timer4

	TCFG0_REG &= ~(0xff << 8);
	TCFG0_REG |= (0x01 << 8); //prescaler:1 => freq:pclk/2

	TCFG1_REG &= ~(0xf << 16); //divider:1

	TCON_REG |= (1<<21) | (1<<22); // auto_reload,update TCNTB4

	TCNTB4_REG = (pclk/2)/1000*interval_ms; //10ms

	irq_register_handler(IRQ_TIMER4_VIC, SYS_TIMER_PRIO, sys_timer_irq);
}

void sys_timer_start(void)
{
	TCON_REG |= (1<<21);//manual update on
	asm volatile ("nop; nop; nop");
	TCON_REG &= ~(1<<21);//manual update off
	TCON_REG |= (1<<20);//start timer4

	TINT_CSTAT_REG |= 1<<4; //enable interrupt
	irq_unmask(IRQ_TIMER4_VIC);
}

void sys_timer_stop(void)
{
	irq_mask(IRQ_TIMER4_VIC);
	TINT_CSTAT_REG &= ~(1<<4); //disable interrupt
	TCON_REG &= ~(1<<20);//stop timer4
}

int sys_timer_attach_handler(event_handler_t *h)
{
	int i;

	for(i = 0; i < MAX_SYS_TIMER_HANDLER; i++) {
		if(sys_timer_handler[i].fn == NULL)
			break;
	}

	if(i == MAX_SYS_TIMER_HANDLER)
		return -ENOMEM;

	sys_timer_handler[i].fn = h->fn;
	sys_timer_handler[i].arg = h->arg;
	return i;
}

void sys_timer_dettach_handler(int fd)
{
	if(fd >= MAX_SYS_TIMER_HANDLER || fd < 0)
		return;

	sys_timer_handler[fd].fn = NULL;
	sys_timer_handler[fd].arg = NULL;
	return;
}

int timer_set(int ms)
{
	

}


int timer_check(int n)
{


}

/* timer0 ~ timer3 */
