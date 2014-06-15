#include <ucos.h>
#include <base.h>

void mdelay(unsigned long n)
{
	n = n*OS_TICKS_PER_SEC/1000;
	OSTimeDly(n); /* check ppp up every 1S */
}

void udelay(unsigned long n)
{
	RASSERT(0);
}


