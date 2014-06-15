/*ucos configure file*/
#ifndef OS_CFG_H
#define OS_CFG_H


/*enable argument check for os functions*/
#define  OS_ARG_CHK_EN  0

/*enable hook funciton*/
#define  OS_CPU_HOOKS_EN  1

/*define the lowest priority*/
#define  OS_LOWEST_PRIO 63

#define  OS_MAX_EVENTS  255

#define  OS_MAX_FLAGS    32

#define  OS_MAX_MEM_PART  1

#define  OS_MAX_QS  32

#define  OS_MAX_TASKS   32

#define  OS_SCHED_LOCK_EN  1

/*has stat task or not*/
#define  OS_TASK_STAT_EN  0

/*os timer run at OS_TICKS_PER_SEC*/
#define  OS_TICKS_PER_SEC 100 

/*idle task size*/
#define  OS_TASK_IDLE_STK_SIZE   4096 /*4K bytes*/

/*stat task size*/
#define  OS_TASK_STAT_STK_SIZE   4096 /*4K bytes*/


/*FLAG*/
#define  OS_FLAG_EN				1
#define  OS_FLAG_WAIT_CLR_EN		1
#define  OS_FLAG_ACCEPT_EN		1
#define  OS_FLAG_DEL_EN			1
#define  OS_FLAG_QUERY_EN			1

/*MBOX*/
#define  OS_MBOX_EN				1
#define  OS_MBOX_ACCEPT_EN		1
#define  OS_MBOX_DEL_EN			1
#define  OS_MBOX_POST_EN			1
#define  OS_MBOX_POST_OPT_EN		1
#define  OS_MBOX_QUERY_EN			1

/*MEMORY*/
#define  OS_MEM_EN					0
#define  OS_MEM_QUERY_EN			0

/*MUTEX*/
#define  OS_MUTEX_EN				1
#define  OS_MUTEX_ACCEPT_EN		1
#define  OS_MUTEX_DEL_EN			1
#define  OS_MUTEX_QUERY_EN		1

/*Q*/
#define  OS_Q_EN					1
#define  OS_Q_ACCEPT_EN			1
#define  OS_Q_DEL_EN				1
#define  OS_Q_FLUSH_EN				1
#define  OS_Q_POST_EN				1
#define  OS_Q_POST_FRONT_EN		1
#define  OS_Q_POST_OPT_EN			1
#define  OS_Q_QUERY_EN				1

/*SEM*/
#define  OS_SEM_EN					1
#define  OS_SEM_ACCEPT_EN			1
#define  OS_SEM_DEL_EN				1
#define  OS_SEM_QUERY_EN			1

/*TASK*/
#define  OS_TASK_CHANGE_PRIO_EN	0
#define  OS_TASK_CREATE_EN		1
#define  OS_TASK_CREATE_EXT_EN	0
#define  OS_TASK_DEL_EN			1
#define  OS_TASK_SUSPEND_EN		1
#define  OS_TASK_QUERY_EN			1

/*TIME*/
#define  OS_TIME_DLY_HMSM_EN		1
#define  OS_TIME_DLY_RESUME_EN	1
#define  OS_TIME_GET_SET_EN		1

#endif

