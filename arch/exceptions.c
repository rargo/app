#include <uart.h>

void c_reset_handler(void)
{
	for(;;)
		serial_puts("c_reset_handler\r\n");
}

void c_undef_handler(void)
{
	for(;;)
		serial_puts("c_undef_handler\r\n");
}


void c_swi_handler(void)
{
	for(;;)
		serial_puts("c_swi_handler\r\n");
}


unsigned long abt_lr;
unsigned long abt_spsr;
unsigned long abt_bad_address;
unsigned long fault_status;
unsigned long fault_address;
void c_dabt_handler(void)
{

#if 0
	__asm__ __volatile__ ("mov	%1,%0\n\t" \
		"str	%3,[%1]\n\t" \
		: "=r" (abt_lr),"=r" (abt_spsr),"=r" (abt_bad_address)
		: 
		: "r14");
#endif

	__asm volatile("stmfd r13!,{r0,r1}");
	__asm volatile("ldr	r0,=abt_lr");
	__asm volatile("str	lr,[r0]");
	__asm volatile("ldr	r0,=abt_spsr");
	__asm volatile("mrs	r1,spsr");
	__asm volatile("str	r1,[r0]");
	__asm volatile("ldr	r0,=fault_status");
	__asm volatile("mrc p15, 0, r1, c5, c0, 0");
	__asm volatile("str	r1,[r0]");
	__asm volatile("ldr	r0,=fault_address");
	__asm volatile("mrc p15, 0, r1, c6, c0, 0");
	__asm volatile("str	r1,[r0]");
	__asm volatile("ldmfd r13!,{r0,r1}");
	

	dprintf("c_dabt_handler\r\n");
	dprintf("lr-8:0x%8x\r\n",abt_lr - 8);
	dprintf("spsr:0x%8x\r\n",abt_spsr);
	dprintf("fault_status:0x%8x\r\n",fault_status);
	dprintf("fault_address:0x%8x\r\n",fault_address);
	for(;;)
		;
}

void c_pabt_handler(void)
{
	for(;;)
		serial_puts("c_pabt_handler\r\n");
}
