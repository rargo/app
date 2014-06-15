#include <ucos.h>
#include <base.h>
#include <log.h>

/*OSTaskStatHook called by task OS_TaskStat
  *OS_TaskStat task priority only higher than idle task
  */
void  OSTaskStatHook(void)
{
    //char_send_string("CPU_USE:");
    //char_send_num(OSCPUUsage);
}

/* called by OSInit before initial os
  */
void  OSInitHookBegin(void)
{
    ;
}

/* called by OSInit after initial os
  */
void  OSInitHookEnd(void)
{
     ;
}

/*called by OSTimeTick before process timer event
  */
void  OSTimeTickHook(void)//定时中断显示"T"
{
	//dprintf("T");
}

/*called by OS_TaskIdle,OS_TaskIdle is lowest priority task.
  */
void  OSTaskIdleHook(void)
{
	/* print log */
	dlog_print();
    //RDIAG(UCOS_DEBUG,"Task Idle");
}

void print_task_stack(unsigned int *p);
/*called by OSIntCtxSw,OSStartHighRdy,OS_TASK_SW when intend to switch task
  */
void OSTaskSwHook(void)
{
	RDIAG(TASK_SW_DEBUG, "<<<=====================TASK %02d", OSPrioCur);
	RDIAG(TASK_SW_DEBUG, "=====================>>>TASK %02d",OSPrioHighRdy);
}


/*called by OSTaskDel */
void OSTaskDelHook(OS_TCB *ptcb)
{
	RDIAG(UCOS_DEBUG, "TASK DELETE:%d",ptcb->OSTCBPrio);
}

/*called by OS_TCBInit when about to creat a task*/
void OSTCBInitHook(OS_TCB *p)
{
	p = p;

	RDIAG(UCOS_DEBUG,"TASK INIT:%d",p->OSTCBPrio);
}

/*called by OS_TCBInit when about to creat a task*/
void  OSTaskCreateHook(OS_TCB *p)
{
	p = p;

	RDIAG(UCOS_DEBUG,"TASK CREATE:%d",p->OSTCBPrio);
}
