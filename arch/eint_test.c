#include <base.h>
#include <uart.h>
#include <timer.h>
#include <irq.h>
#include <eint.h>

void eint_h0(void)
{
	dprintf("0\r\n");
}
void eint_h1(void)
{
	dprintf("1\r\n");
}
void eint_h2(void)
{
	dprintf("2\r\n");
}
void eint_h3(void)
{
	dprintf("3\r\n");
}
void eint_h4(void)
{
	dprintf("4\r\n");
}
void eint_h5(void)
{
	dprintf("5\r\n");
}
void eint_h19(void)
{
	dprintf("19\r\n");
}
void eint_h20(void)
{
	dprintf("20\r\n");
}

void eint_test(void)
{
	int i;
	volatile unsigned long *p;

	eint_init(3, 5, 6, 7);

	eint_set_type(EINT_CON0_1, EINT_TRIGGER_FALLING_EDGE, \
			EINT_FILTER_DELAY, 0);
	eint_set_type(EINT_CON2_3, EINT_TRIGGER_FALLING_EDGE, \
			EINT_FILTER_DELAY, 0);
	eint_set_type(EINT_CON4_5, EINT_TRIGGER_FALLING_EDGE, \
			EINT_FILTER_DELAY, 0);
	eint_set_type(EINT_CON18_19, EINT_TRIGGER_FALLING_EDGE, \
			EINT_FILTER_DELAY, 0);
	eint_set_type(EINT_CON20_21, EINT_TRIGGER_FALLING_EDGE, \
			EINT_FILTER_DELAY, 0);

	eint_set_gpio(EINT(0),0);
	eint_set_gpio(EINT(1),0);
	eint_set_gpio(EINT(2),0);
	eint_set_gpio(EINT(3),0);
	eint_set_gpio(EINT(4),0);
	eint_set_gpio(EINT(5),0);
	eint_set_gpio(EINT(19),0);
	eint_set_gpio(EINT(20),0);

	eint_set_handler(EINT(0), eint_h0);
	eint_set_handler(EINT(1), eint_h1);
	eint_set_handler(EINT(2), eint_h2);
	eint_set_handler(EINT(3), eint_h3);
	eint_set_handler(EINT(4), eint_h4);
	eint_set_handler(EINT(5), eint_h5);
	eint_set_handler(EINT(19), eint_h19);
	eint_set_handler(EINT(20), eint_h20);

	eint_single_unmask(EINT(0));
	eint_single_unmask(EINT(1));
	eint_single_unmask(EINT(2));
	eint_single_unmask(EINT(3));
	eint_single_unmask(EINT(4));
	eint_single_unmask(EINT(5));
	eint_single_unmask(EINT(19));
	eint_single_unmask(EINT(20));

	i=0;
	p = &EINTCON0_REG;

	dprintf("%d: 0x%8x\r\n",i++, *p++);
	dprintf("%d: 0x%8x\r\n",i++, *p++);

	p = &EINTMASK_REG;
	dprintf("%d: 0x%8x\r\n",i++, *p++);
	dprintf("%d: 0x%8x\r\n",i++, *p++);
}
