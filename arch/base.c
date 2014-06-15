#include <base.h>


#define M *1000*1000

/* becareful of overflow when using fclk,hclk,pclk */
unsigned int fclk = 532 M;
unsigned int hclk = 133 M;
unsigned int pclk = 66 M;

int errno;

void debug_s_irq_handler_out(unsigned int *stack)
{
	dprintf("IRQ OUT STACK:\r\n");
	dprintf("sp:0x%8x\r\n", (unsigned int)stack);
	dprintf("spsr:0x%8x\r\n", *stack++);
	dprintf("r0:0x%8x\r\n", *stack++);
	dprintf("r1:0x%8x\r\n", *stack++);
	dprintf("r2:0x%8x\r\n", *stack++);
	dprintf("r3:0x%8x\r\n", *stack++);
	dprintf("r4:0x%8x\r\n", *stack++);
	dprintf("r5:0x%8x\r\n", *stack++);
	dprintf("r6:0x%8x\r\n", *stack++);
	dprintf("r8:0x%8x\r\n", *stack++);
	dprintf("r9: 0x%8x\r\n", *stack++);
	dprintf("r10:0x%8x\r\n", *stack++);
	dprintf("r11:0x%8x\r\n", *stack++);
	dprintf("r7:0x%8x\r\n", *stack++);
	dprintf("r12:0x%8x\r\n", *stack++);
	dprintf("lr:0x%8x\r\n", *stack++);
	dprintf("\r\n");
}

void debug_s_irq_handler_in(unsigned int *stack)
{
	dprintf("IRQ IN STACK:\r\n");
	dprintf("sp:0x%8x\r\n", (unsigned int)stack);
	dprintf("spsr:0x%8x\r\n", *stack++);
	dprintf("r0:0x%8x\r\n", *stack++);
	dprintf("r1:0x%8x\r\n", *stack++);
	dprintf("r2:0x%8x\r\n", *stack++);
	dprintf("r3:0x%8x\r\n", *stack++);
	dprintf("r4:0x%8x\r\n", *stack++);
	dprintf("r5:0x%8x\r\n", *stack++);
	dprintf("r6:0x%8x\r\n", *stack++);
	dprintf("r8:0x%8x\r\n", *stack++);
	dprintf("r9: 0x%8x\r\n", *stack++);
	dprintf("r10:0x%8x\r\n", *stack++);
	dprintf("r11:0x%8x\r\n", *stack++);
	dprintf("r7:0x%8x\r\n", *stack++);
	dprintf("r12:0x%8x\r\n", *stack++);
	dprintf("lr:0x%8x\r\n", *stack++);
	dprintf("\r\n");
}


/* cpu frequency counter:
 * cycle couter freq: 532M/64, per count takes 120ns 
 */
void cpu_counter_init(void)
{
	unsigned long count;
	/* bit1 = 1:enable counter, bit3 = 1:cycle couter is fclk/64 */
	asm volatile("mrc p15, 0, r0, c15, c12, 0\n\t"
				"orr r0,r0,#0x9\n\t"
				"mcr p15,0, r0, c15, c12, 0\n\t"
			);
}

/* cpu frequency counter */
unsigned int cpu_counter(void)
{
	unsigned long count;
	asm volatile("mrc p15, 0, %0, c15, c12, 1\n\t"
			: "=r"(count));

	return count;
}

/* in s,assume cpu counter not overflow, careful: max 5.1S */
unsigned int cpu_counter_ns(unsigned int stop, unsigned int start)
{
	if(stop >= start)
		return (stop - start) * 120;
	else
		return (0xfffffffUL - (start - stop)) * 120;
}

void sys_tick(void)
{


}

void DI(void)
{
	__asm volatile ("mrs	r1,cpsr\t\n"
			"orr	r1,r1,#0xc0	\t\n"
			"msr	cpsr_c,r1\t\n"
			);
}

void EI(void)
{
	__asm volatile ("mrs	r1,cpsr\t\n"
			"bic	r1,r1,#0xc0	\t\n"
			"msr	cpsr_c,r1\t\n"
			);
}

int irq_nest_count(void)
{
	extern unsigned char OSIntNesting;
	return OSIntNesting;
}

#define MAX_SOFTIRQ 32
static volatile int softirq_runing = 0;
static volatile unsigned int softirq_pending = 0;
struct softirq_handler {
	void (*h)(void);
};
struct softirq_handler _softirq_handler[MAX_SOFTIRQ];
void do_softirq(void)
{
	int softirq_restart;

	/* only allow one sofrirq instance runing */
	if(softirq_runing)
		return;
	if(softirq_pending == 0)
		return;
	/*check if last level of irq */
	if(irq_nest_count() != 1)
		return;
	softirq_runing = 1;
	//RDIAG(SOFTIRQ_DEBUG);

	softirq_restart = 0;
	for(;;) {
		int i;
		unsigned int pending;

		DI();
		pending = softirq_pending;
		softirq_pending = 0;
		//RDIAG(SOFTIRQ_DEBUG,"pending:%8x",pending);
		EI();

		if(pending == 0)
			break;

		i = 0;
		while(pending) {
			if(pending & 0x01) {
				if(_softirq_handler[i].h != NULL)
					(_softirq_handler[i].h)();
				else
					RERR("SOFTIRQ %d pending, but no handler",i);
			}
			++i;
			pending >>= 1;
		}

		if(++softirq_restart > 10)
			RERR("SOFTIRQ OVERLOAD!");
	}

	DI();
	//RDIAG(SOFTIRQ_DEBUG);
	softirq_runing = 0;
}

void softirq_init(void)
{
	memset(_softirq_handler, 0, sizeof(_softirq_handler));
}

/* return -1 if fail,else return softirq_handler use for raise_softirq() */
int softirq_require(void (*h)(void))
{
	int i;
	for(i=0;i<MAX_SOFTIRQ;i++) {
		if(_softirq_handler[i].h != NULL)
			continue;
		_softirq_handler[i].h = h;
		return i;
	}

	return -1;
}

/* must be call in irq with irq disable */
int softirq_raise(int n)
{
	if(n < 0 || n >= MAX_SOFTIRQ) {
		RERR("softirq nr error");
		return -1;
	}

	softirq_pending |= 1UL << n;
	return 0;
}

