#include <base.h>
#include <irq.h>
#include <eint.h>

/* function handle extern gpio group0 interrupts */

static void (*eint_handler[EINT_NUM])(void);


/* 注意:
 * 6410的EINT_PEND寄存器处于EINT_MASK_REG的前面，即使用EINT_MASK_REG
 * 并不能使EINT_PEND寄存器中的位停止置位,如果使用的是电平触发，
 * 有很大的风险.
 * 1.
 *		eint_single_mask(i);
 *		if(eint_handler[i])
 *			(eint_handler[i])();  //这里有可能产生了新的中断，
 *								//下面的一句误将其清除,导致丢失了一次中断
 *		EINTPEND_REG |= 1UL<<i; 
 *		eint_single_unmask(i);
 *
 * 2.
 *		eint_single_mask(i);
 *		EINTPEND_REG |= 1UL<<i;  //会导致两次中断的产生,这里清除了标志,
 *		//但是因为EINT_MASK_REG寄存器不能是EINT_PEND寄存器停止置位
 *		//如果是电平未改变，这里则马上再次置位
 *		if(eint_handler[i])
 *			(eint_handler[i])();  
 *		eint_single_unmask(i);
 */
unsigned int eint_single_mask(int n);
unsigned int eint_single_unmask(int n);
static void eint_handler_commmon(unsigned int i)
{
	if(EINTPEND_REG & (1UL<<i)) {
		eint_single_mask(i);
		/* note, code as below only support edge trigger now,
		 * level trigger will happen twice
		*/
		EINTPEND_REG |= 1UL<<i; /* clear interrupt */
		if(eint_handler[i])
			(eint_handler[i])(); 
		eint_single_unmask(i);
	}
}

/*
 * 如果一个vic通道只为一个外设服务，并且该外设没有屏蔽其产生
 * irq请求的寄存器，则其irq处理程序为:
 * 1.屏蔽vic中该外设的请求
 * 2.清除该外设此次的中断请求(ack此次中断请求)
 * 3.调用服务程序
 * 4.使能vic中该外设的请求
 *
 * 如果一个vic通道为几个外设(同种或不同种)服务，并且该外设有单独
 * 控制其产生irq请求的的寄存器，则其irq处理程序为:
 * 1. 通过外设寄存器屏蔽该外设的请求,
 *    (防止在3服务程序清除中断源前，2ack此次请求不成功，即无法清除此次的中断标志位)
 * 2. 清除该外设此次的中断请求(ack此次中断请求)
 * 3. 调用服务程序
 *	  (服务程序应首先清除此次中断产生的原因)
 * 4. 通过外设寄存器重新允许该外设的请求
 * 注意vic的输入均为电平触发
 */
static void eint0_3_isr(void)
{
	int i;
	
#if 0
	RDIAG(LEGACY_DEBUG,"EINTPEND_REG:0x%8x", EINTPEND_REG);
#endif
	for(i=0; i<=3; i++)
		eint_handler_commmon(i);
}

static void eint4_11_isr(void)
{
	int i;
	
#if 0
	RDIAG(LEGACY_DEBUG,"EINTPEND_REG:0x%8x", EINTPEND_REG);
#endif
	for(i=4; i<=11; i++)
		eint_handler_commmon(i);
}

static void eint12_19_isr(void)
{
	int i;
#if 0
	RDIAG(LEGACY_DEBUG,"EINTPEND_REG:0x%8x", EINTPEND_REG);
#endif
	for(i=12; i<=19; i++)
		eint_handler_commmon(i);
}

static void eint20_27_isr(void)
{
	int i;
#if 0
	RDIAG(LEGACY_DEBUG,"EINTPEND_REG:0x%8x", EINTPEND_REG);
#endif
	for(i=20; i<=27; i++)
		eint_handler_commmon(i);
}

void eint_init(int eint0_3_prio,int eint4_11_prio, int eint12_19_prio, int eint20_27_prio)
{
	EINTMASK_REG = ~0; /* mask all gpio int */

	memset(eint_handler, 0 ,sizeof(eint_handler));

	irq_register_handler(IRQ_EINT0_3, eint0_3_prio, eint0_3_isr);
	irq_register_handler(IRQ_EINT4_11, eint4_11_prio, eint4_11_isr);
	irq_register_handler(IRQ_EINT12_19, eint12_19_prio, eint12_19_isr);
	irq_register_handler(IRQ_EINT20_27, eint20_27_prio, eint20_27_isr);

	/* unmask eint in vic */
	irq_unmask(IRQ_EINT0_3);
	irq_unmask(IRQ_EINT4_11);
	irq_unmask(IRQ_EINT12_19);
	irq_unmask(IRQ_EINT20_27);
}

/* return previous exterrupt interrupt mask */
unsigned int eint_single_mask(int n)
{
	unsigned int sr;
	unsigned int save_mask;

	local_irq_save(sr);
	save_mask = EINT_MASK_REG;
	EINTMASK_REG |= 1<<n;
	local_irq_restore(sr);

	return save_mask;
}

unsigned int eint_single_unmask(int n)
{
	unsigned int sr;
	unsigned int save_mask;

	local_irq_save(sr);
	save_mask = EINT_MASK_REG;
	EINTMASK_REG &= ~(1<<n);
	local_irq_restore(sr);

	return save_mask;
}

void eint_set_handler(int n, void (*handler)(void))
{
	unsigned int sr;

	if(n >= EINT_NUM)
		return;

	local_irq_save(sr);
	eint_handler[n] = handler;
	local_irq_restore(sr);
}

/*
 * eint_n n : EINT_CON0_1 ...
 * eint_type type:EINT_TRIGGER_LOW ...
 * eint_filter filter: EINT_FILTER_DISABLE ...
 * int digital_filter_count: only use when filter is EINT_FILTER_DIGITAL
 */
void eint_set_type(eint_n n, eint_type type, eint_filter filter, int digital_filter_count)
{
	unsigned int filter_n;
	unsigned int reg;
	volatile unsigned long *eint_con_reg;
	volatile unsigned long *eint_flt_reg;

	if(n <= EINT_CON14_15) {
		eint_con_reg = &EINTCON0_REG; 
		if(n <= EINT_CON6_7) {
			filter_n = n;
			eint_flt_reg = &EINTFLTCON0_REG;
		} else {
			filter_n = n - EINT_CON6_7;
			eint_flt_reg = &EINTFLTCON1_REG;
		}
	} else if(n <= EINT_CON26_27) {
		n -= EINT_CON14_15;
		eint_con_reg = &EINTCON1_REG; 
		if(n <= EINT_CON6_7) {
			filter_n = n;
			eint_flt_reg = &EINTFLTCON2_REG;
		} else {
			filter_n = n - EINT_CON6_7;
			eint_flt_reg = &EINTFLTCON3_REG;
		}
	} else
		return;

	if(type == EINT_TRIGGER_HIGH || type == EINT_TRIGGER_LOW) {
		RERR("EINT IS LOW OR HIGH TRIGGER, VERY RISK!!!!");
	}

	reg = *eint_con_reg;
	reg &= ~(0x07 << (n*4));
	reg |= type << (n*4);
	*eint_con_reg = reg;

	switch(filter)
	{
	case EINT_FILTER_DISABLE:
		*eint_flt_reg &= ~(0x80 << filter_n*8);
		break;
	case EINT_FILTER_DELAY:
		*eint_flt_reg |= 0x80 << filter_n*8;
		*eint_flt_reg &= ~(0x40 << filter_n*8);
		break;
	case EINT_FILTER_DIGITAL:
		*eint_flt_reg |= 0x80 << filter_n*8;
		*eint_flt_reg |= 0x40 << filter_n*8;
		*eint_flt_reg &= ~(0x3f << filter_n*8);
		*eint_flt_reg |= digital_filter_count << filter_n*8;
		break;
	default:
		break;
	}
}

/* set conresponding gpio to extern interrupt mode, n: eint number
 * pull_up_down:
 *	0: disable pull up or pull down
 *	1: pull down
 *	2: pull up
 */
void eint_set_gpio(int n, int pull_up_down)
{
	if(n <= 15) {
		GPNCON_REG &= ~(0x03 << n*2);
		GPNCON_REG |= 0x02 << n*2;

		GPNPUD_REG &= ~(0x03 << n*2);
		GPNPUD_REG |= (pull_up_down << n*2);
	} else if( n <= 22) {
		n -= 22;
		GPLCON1_REG &= ~(0x0f << n*4);
		GPLCON1_REG |= (0x03 << n*4);

		GPLPUD_REG &= ~(0x03 << n*2);
		GPLPUD_REG |= (pull_up_down << n*2);
	} else if( n <= 27) {
		n -= 27;
		GPMCON_REG &= ~(0x0f << n*4);
		GPMCON_REG |= (0x03 << n*4);

		GPMPUD_REG &= ~(0x03 << n*2);
		GPMPUD_REG |= (pull_up_down << n*2);
	} else
		return;
}
