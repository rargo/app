#include <ucos.h>
#include <base.h>
#include <uart.h>
#include <timer.h>
#include <lwip/sys.h> /*lwip sys.h*/

#if 0
static OS_EVENT *mbox;
static sys_mbox_t sys_mbox;
static sys_sem_t sys_sem;
static sys_mutex_t sys_mutex;

static void *queue[16];

static unsigned  int  task1_stk[1024];
static unsigned  int  task2_stk[1024];
static unsigned  int  task3_stk[1024];

static void task1(void *pdata)
{
	int timeout;
	int i;
	INT8U err;
	char *p;

    while(1)
    {
        dprintf("task1,%s\r\n",pdata);
		//dprintf("sys_jiffies:%d\r\n",sys_jiffies());
		//dprintf("sys_now:%dms\r\n",sys_now());
		//p = OSQPend(mbox, 1, &err);
		timeout = sys_arch_mbox_fetch(&sys_mbox, (void **)&p, 0);
		sys_mutex_lock(&sys_mutex);
		if(timeout != -1)
			dprintf("task1 received mbox: %s\r\n",p);
		sys_mutex_unlock(&sys_mutex);
		//else
			//OSTaskResume(9);
#if 0
		if(err != OS_NO_ERR)
		{
			dprintf("task1 OSQPend err:%d\r\n",err);
			OSTaskResume(9);
		}
		else
			dprintf("task1 received mbox: %s\r\n",p);
#endif
    }
}

/* prio:10 */
static void task2(void *pdata)
{
	
	char *m1 = "task2 message1";
	char *m2 = "task2 message2";
	char *m3 = "task2 message3";
	char *m4 = "task2 message4";
    while(1)
    {
    		sys_mutex_lock(&sys_mutex);
		dprintf("task2,%s\r\n",pdata);
#if 0
		dprintf("task2 post m1\r\n");
		OSQPost(mbox, m1);
		dprintf("task2 post m2\r\n");
		OSQPost(mbox, m2);
		dprintf("task2 post m3\r\n");
		OSQPost(mbox, m3);
		dprintf("task2 post m4\r\n");
		OSQPost(mbox, m4);
#endif
		dprintf("task2 post message1\r\n");
		sys_mbox_post(&sys_mbox, m1);
		dprintf("task2 post message2\r\n");
		sys_mbox_post(&sys_mbox, m2);
		dprintf("task2 post message3\r\n");
		sys_mbox_post(&sys_mbox, m3);
		dprintf("task2 post message4\r\n");
		sys_mbox_post(&sys_mbox, m4);
		sys_mutex_unlock(&sys_mutex);
	    //OSTaskSuspend(OS_PRIO_SELF);
		//sys_sem_wait(&sys_sem);
		
	    //OSTaskResume(9);
    }
}

static void task3(void *pdata)
{
	unsigned char *p;
	
    while(1)
	{
	  dprintf("task3,%s\r\n",pdata);
	p = malloc(234);
	dprintf("p:0x%8x\r\n",p);
	free(p);
	//sys_mutex_unlock(&sys_mutex);
      //sys_sem_signal(&sys_sem);
      
       // OSTaskResume(10);
    }
}

static unsigned int debug_enable = 0;

void ucos_test(void)
{
	char *str1 = "!";
	char *str2 = "?";

	
   OSInit();
	//mbox = OSQCreate (queue, sizeof(queue)/sizeof(queue[0]));
	//dprintf("OSQCreate return:0x%8x\r\n",mbox);
	
	/*install os timer*/	
	//sys_timer_start();
    //OSTaskCreate(task1,(void *)str1,&task1_stk[1024],11);
    //OSTaskCreate(task2,(void *)str2,&task2_stk[1024],10);
    //OSTaskCreate(task3,(void *)"*",&task3_stk[1024],11);
    //debug_enable = 1;
	sys_thread_new("thread1", task1, str1, 1024, 8);
	sys_thread_new("thread2", task2, str2, 1024, 9);
	sys_thread_new("thread3", task3, str2, 1024, 10);
	sys_mbox_new(&sys_mbox, 30);
	//sys_sem_new(&sys_sem, 0);
	sys_mutex_new(&sys_mutex);
    OSStart();
	
    while(1)
    {
        dprintf("os runtime error");
    }
}

#else

static unsigned  int  task1_stk[1024];
static unsigned  int  task2_stk[1024];
static unsigned  int  task3_stk[1024];

static void task1(void *pdata)
{
    while(1)
    {
        dprintf("task1,%s\r\n",pdata);
		OSTaskSuspend(OS_PRIO_SELF);
    }
}

/* prio:10 */
static void task2(void *pdata)
{
	
    while(1)
    {
		dprintf("task2,%s\r\n",pdata);
		OSTaskSuspend(OS_PRIO_SELF);
		OSTaskResume(9);
    }
}

static void task3(void *pdata)
{
	unsigned char *p;
	
    while(1)
	{
		dprintf("task3,%s\r\n",pdata);
		OSTaskResume(10);
    }
}

static unsigned int debug_enable = 0;

void ucos_test(void)
{
	char *str1 = "!";
	char *str2 = "?";
	
   OSInit();
	/*install os timer*/	
	//sys_timer_start();
    OSTaskCreate(task1,(void *)str1,&task1_stk[1024],9);
    OSTaskCreate(task2,(void *)str2,&task2_stk[1024],10);
    OSTaskCreate(task3,(void *)"*",&task3_stk[1024],11);
    OSStart();
	
    while(1)
    {
        dprintf("os runtime error");
    }
}
#endif

