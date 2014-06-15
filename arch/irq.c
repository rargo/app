#include <base.h>
#include <irq.h>

#define VIC0_ADDR 0x71200000
#define VIC1_ADDR 0x71300000

#define VIC_IRQ_STATUS			0x00
#define VIC_FIQ_STATUS			0x04
#define VIC_RAW_STATUS			0x08

#define VIC_INT_SELECT				0x0c	/* 1 = FIQ, 0 = IRQ */
#define VIC_INT_ENABLE			0x10	/* 1 = enable, 0 = disable */
#define VIC_INT_ENABLE_CLEAR		0x14
#define VIC_INT_SOFT				0x18
#define VIC_INT_SOFT_CLEAR		0x1c
#define VIC_PROTECT				0x20
#define VIC_SOFT_PRIORITY_MASK	0x24
#define VIC_PRIORITY_DAISY		0x28
#define VIC_VECT_ADDR0			0x100	/* 0..31 */
#define VIC_VECT_PRIORITY0		0x200	/* 0..31 */
#define VIC_VECT_CNTL_ENABLE		(1 << 5)

#define VIC_VECT_ADDR		0xF00

/* vic vector address is index to irq_c_handler[] 
 */
static void (*c_handler[IRQ_MAX])(void);
void irq_c_handler(unsigned int irq)
{
	//RDIAG(LEGACY_DEBUG,"irq:%d",irq);
	RASSERT(irq < IRQ_MAX);

	irq_mask(irq);
	if(c_handler[irq] != NULL)
		(*c_handler[irq])();
	irq_unmask(irq);
}

void irq_init(void)
{
	int i;

	/* vic0 init */

	writel(0, VIC0_ADDR + VIC_INT_SELECT);
	writel(0, VIC0_ADDR + VIC_INT_ENABLE);
	writel(~0, VIC0_ADDR + VIC_INT_ENABLE_CLEAR);
	writel(0, VIC0_ADDR + VIC_IRQ_STATUS);
	writel(~0, VIC0_ADDR + VIC_INT_SOFT_CLEAR);

	/* invalidate all vector */
	for(i=0; i<32; i++)
		writel(IRQ_MAX, VIC0_ADDR + VIC_VECT_ADDR0 + i*4);
	writel(IRQ_MAX, VIC0_ADDR + VIC_VECT_ADDR);

	/* unmask all prioriy */
	writel(0xffff, VIC0_ADDR + VIC_SOFT_PRIORITY_MASK);

	/* set vic0 daisy priority to 8 
	 * from high to low,priority sequence is: 
	 *		vic0 priority 0~7;vic1 priority 0~15,vic0 priority 8; vic0 9~15
	 * */
	writel(0x08, VIC0_ADDR + VIC_PRIORITY_DAISY);


	/* vic1 init */
	writel(0, VIC1_ADDR + VIC_INT_SELECT);
	writel(0, VIC1_ADDR + VIC_INT_ENABLE);
	writel(~0, VIC1_ADDR + VIC_INT_ENABLE_CLEAR);
	writel(0, VIC1_ADDR + VIC_IRQ_STATUS);
	writel(~0, VIC1_ADDR + VIC_INT_SOFT_CLEAR);

	/* invalidate vic1 all vector */
	for(i=0; i<32; i++)
		writel(IRQ_MAX, VIC1_ADDR + VIC_VECT_ADDR0 + i*4);

	writel(IRQ_MAX, VIC1_ADDR + VIC_VECT_ADDR);

	/* unmask all prioriy */
	writel(0xffff, VIC1_ADDR + VIC_SOFT_PRIORITY_MASK);

	/* initial all irq to be invalid */
	memset(c_handler, IRQ_MAX, sizeof(c_handler));
}

int irq_register_handler(unsigned int intr, unsigned int prio, void (*handler)(void))
{
	if(intr >= 64)
		return -EINVAL;

	c_handler[intr] = handler;

	if(intr < 32) {
		writel(intr, VIC0_ADDR + VIC_VECT_ADDR0 + intr*4);
		writel(prio, VIC0_ADDR + VIC_VECT_PRIORITY0 + intr*4);
	} else {
		intr -= 32;
		writel(intr + 32, VIC1_ADDR + VIC_VECT_ADDR0 + intr*4);
		writel(prio, VIC1_ADDR + VIC_VECT_PRIORITY0 + intr*4);
	}

	return 0;
}

void irq_triger_soft(unsigned int intr)
{
	if(intr >= IRQ_MAX)
		return;

	if(intr < 32)
		writel(1<<intr, VIC0_ADDR + VIC_INT_SOFT);
	else {
		intr -= 32;
		writel(1<<intr, VIC1_ADDR + VIC_INT_SOFT);
	}
}

void irq_mask(unsigned int intr)
{
	if(intr >= 64)
		return;

	if(intr < 32)
		writel(1UL<<intr, VIC0_ADDR + VIC_INT_ENABLE_CLEAR);
	else {
		intr -= 32;
		writel(1UL<<intr, VIC1_ADDR + VIC_INT_ENABLE_CLEAR);
	}
}

void irq_unmask(unsigned int intr)
{
	if(intr >= 64)
		return;

	if(intr < 32)
		writel(1UL<<intr, VIC0_ADDR + VIC_INT_ENABLE);
	else {
		intr -= 32;
		writel(1UL<<intr, VIC1_ADDR + VIC_INT_ENABLE);
	}
}
