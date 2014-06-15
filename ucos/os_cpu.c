
/*ucos port,task save context in stack,from top to bottom:pc,lr,r12,r11......r0,cpsr*/
#include "ucos.h"
#include <base.h>
//#include <linux/string.h>

void dprintf_task_switch(void)
{
	dprintf("TS prev task prio:%d,next task prio:%d,task:%d\r\n",OSPrioCur,OSPrioHighRdy,OSTaskCtr);
}

void print_task_stack(unsigned int *p)
{
	dprintf("task sp:0x%8x\r\n",p);
	dprintf("\t\tr13_usr:0x%8x\r\n",*p++);
	dprintf("\t\tr14_usr:0x%8x\r\n",*p++);
	dprintf("\t\tcpsr:0x%8x\r\n",*p++);
	dprintf("\t\tr0:0x%8x\r\n",*p++);
	dprintf("\t\tr4:0x%8x\r\n",*p++);
	dprintf("\t\tr5:0x%8x\r\n",*p++);
	dprintf("\t\tr6:0x%8x\r\n",*p++);
	dprintf("\t\tr7:0x%8x\r\n",*p++);
	dprintf("\t\tr8:0x%8x\r\n",*p++);
	dprintf("\t\tr9:0x%8x\r\n",*p++);
	dprintf("\t\tr10:0x%8x\r\n",*p++);
	dprintf("\t\tr11:0x%8x\r\n",*p++);
	dprintf("\t\tr14:0x%8x\r\n",*p++);
}

/*calley by OSTaskCreate,OSTaskCreateEX. initial task stack when task creating*/

/*task stack registers:
*		r14_svc(pc)
*		r4~r11
*		r0
*		cpsr
*		r14_usr
*		r13_usr
*/
OS_STK *OSTaskStkInit(void (*task)(void *pd),void  *pdata,OS_STK *ptos,INT16U opt)
{
	opt = opt;

	/*arm stack operation using stmfd,so here is:
	 *		*(--ptos) = (unsigned int task)
	 */
	*(--ptos) = (unsigned int)task;//pc = task
	ptos -= 8;
	memset((char *)ptos,0,8*4);//r4-r11 = 0
	*(--ptos) = (unsigned int)pdata;//task init r0 = pdata
	*(--ptos) = 0x13; //cpsr: svc mode with irq enable
	/* kernel task, don't care about r13_usr, r14_usr */
	*(--ptos) = 0; 
	*(--ptos) = 0;

	return ptos;
}

extern void SwitchToUserMode(void);
void SwitchToUserMode(void)
{

}
/* create a user application,when task run first time, SwitchToUserMode() will be run,
*  SwitchToUserMode() will switch to user function usr_task().
* para:
*	@usr_task user task
*	@usr_ptos user task stack top, must point to user space
*	@usr_pdata user task parameter, must point to user space
*/
OS_STK *OSUsrTaskStkInit(void (*usr_task)(void *pd),void  *usr_pdata,OS_STK *usr_ptos, OS_STK *ptos, INT16U opt)
{
	opt = opt;

	/*arm stack operation using stmfd,so here is:
	 *		*(--ptos) = (unsigned int task)
	 */
	*(--ptos) = (unsigned int)SwitchToUserMode;//pc = task
	ptos -= 8;
	memset((char *)ptos,0,8*4);//r4-r11 = 0
	*(--ptos) = (unsigned int)usr_pdata;//task init r0 = pdata
	*(--ptos) = 0x13; //cpsr: svc mode with irq enable
	/* kernel task, don't care about r13_usr, r14_usr */
	*(--ptos) = (unsigned int)(usr_ptos); 
	*(--ptos) = (unsigned int)(usr_task);

	return ptos;
}

