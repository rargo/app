#include <base.h>
#include <ucos.h>
#include <arch/sys_arch.h>

#define ERR_OK 0

/** Create a new mbox of specified size
 * @param mbox pointer to the mbox to create
 * @param size (miminum) number of messages in this mbox
 * @return ERR_OK if successful, another err_t otherwise */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	void **p;
	p = (void *)malloc(sizeof(void *) * size);
	if(p == NULL)
		return -ENOMEM;

	mbox->event = OSQCreate(p, size);
	if(mbox->event == NULL) {
		RERR();
		return -ENOMEM;
	}

	mbox->malloc_start = p;
	mbox->valid = 1;
	return ERR_OK;
}


/** Post a message to an mbox - may not fail
 * -> blocks if full, only used from tasks not from ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	u8_t error;
	for(;;) {
		error = OSQPost(mbox->event, msg);
		if(error == OS_NO_ERR)
			break;
		else if(error == OS_Q_FULL)
			continue;
		else
			RERR();
	}
}


/** Try to post a message to an mbox - may fail if full or ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	u8_t error;
	error = OSQPost(mbox->event, msg);
	if(error == OS_NO_ERR)
		return ERR_OK;
	else  {
		RERR("ret:%2x",error);
		return -ENOMEM; /* XXX fix return value */
	}
}

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter! */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	u8_t error;
	u32_t prev_time;
	u32_t now_time;

	if(timeout != 0) {
		if(timeout < SYS_TIMER_INTERVAL)
			timeout = SYS_TIMER_INTERVAL;
		timeout /= SYS_TIMER_INTERVAL;
	}

	prev_time = OSTimeGet();
	RDIAG(MBOX_DEBUG,"mbox try fetch,time:%d(timeout:%d)",prev_time, timeout);
	*msg = OSQPend(mbox->event, timeout, &error);
	if(error == OS_NO_ERR) {
		now_time = OSTimeGet();
		RDIAG(MBOX_DEBUG,"mbox fetch success time:%d",OSTimeGet());
		return (now_time - prev_time)*SYS_TIMER_INTERVAL;
	} else if(error == OS_TIMEOUT) {
		/* notes:
		 * if queue was delete by OSQDel, 
		 * OSQPend will also return OS_TIMEOUT.
		 */
		 RDIAG(MBOX_DEBUG,"mbox fetch timeout! time:%d",OSTimeGet());
		return SYS_ARCH_TIMEOUT;
	} else {
		RERR("error:%2x",error);
	}

	/* NOT REACH */
	return SYS_ARCH_TIMEOUT;
}

/* Allow port to override with a macro, e.g. special timout for sys_arch_mbox_fetch() */
/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	*msg = OSQAccept(mbox->event);

	if(*msg != NULL)
		return 0;
	return SYS_ARCH_TIMEOUT;
}

/** Delete an mbox
 * @param mbox mbox to delete */
void sys_mbox_free(sys_mbox_t *mbox)
{
	u8_t error;
	OSSchedLock();
	mbox->valid = 0;
	OSQDel(mbox->event,OS_DEL_ALWAYS, &error);
	if(error != OS_NO_ERR)
		RERR();
	free(mbox->malloc_start);
	OSSchedUnlock();
}

/** Check if an mbox is valid/allocated: return 1 for valid, 0 for invalid */
int sys_mbox_valid(sys_mbox_t *mbox)
{
	return mbox->valid;
}

/** Set an mbox invalid so that sys_mbox_valid returns 0 */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->valid = 0;
}

/** Create a new semaphore
 * @param sem pointer to the semaphore to create
 * @param count initial count of the semaphore
 * @return ERR_OK if successful, another err_t otherwise */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	OS_EVENT *p;
	p = OSSemCreate(count);
	if(p != NULL) {
		sem->event = p;
		sem->valid = 1;
		return ERR_OK;
	}

	RERR();
	return -ENOMEM;
}


/** Signals a semaphore
 * @param sem the semaphore to signal */
void sys_sem_signal(sys_sem_t *sem)
{
	u8_t error;
	error = OSSemPost(sem->event);
	if(error != OS_NO_ERR)
		RERR();
}


/** Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or SYS_ARCH_TIMEOUT on timeout */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	u8_t error;
	u32_t prev_time;
	u32_t now_time;

	if(timeout != 0) {
		if(timeout < SYS_TIMER_INTERVAL)
			timeout = SYS_TIMER_INTERVAL;
		timeout /= SYS_TIMER_INTERVAL;
	}

	prev_time = OSTimeGet();
	OSSemPend(sem->event, timeout, &error);
	if(error == OS_NO_ERR) {
		now_time = OSTimeGet();
		return (now_time - prev_time)*SYS_TIMER_INTERVAL;
	} else if(error == OS_TIMEOUT) {
		/* if queue was delete by OSQDel, 
		 * OSQPend will return OS_TIMEOUT.
		 */
		return SYS_ARCH_TIMEOUT;
	} else {
		RERR();
	}

	/* NOT REACH */
	return SYS_ARCH_TIMEOUT;
}


/** Delete a semaphore
 * @param sem semaphore to delete */
void sys_sem_free(sys_sem_t *sem)
{
	u8_t error;
	OSSchedLock();
	sem->valid = 0;
	OSSemDel(sem->event, OS_DEL_ALWAYS, &error);
	if(error != OS_NO_ERR)
		RERR();
	OSSchedUnlock();
}


/** Wait for a semaphore - forever/no timeout */
//#define sys_sem_wait(sem)                  sys_arch_sem_wait(sem, 0)
void sys_sem_wait(sys_sem_t *sem)
{
	sys_arch_sem_wait(sem, 0);
}


/** Check if a sempahore is valid/allocated: return 1 for valid, 0 for invalid */
int sys_sem_valid(sys_sem_t *sem)
{
	return sem->valid;
}

/** Set a semaphore invalid so that sys_sem_valid returns 0 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->valid = 0;
}

#if 0
/* use semaphore to implement mutex, see below notes */
err_t sys_mutex_new(sys_mutex_t *mutex)
#define sys_mutex_t                   sys_sem_t
#define sys_mutex_new(mutex)          sys_sem_new(mutex, 1)
#define sys_mutex_lock(mutex)         sys_sem_wait(mutex)
#define sys_mutex_unlock(mutex)       sys_sem_signal(mutex)
#define sys_mutex_free(mutex)         sys_sem_free(mutex)
#define sys_mutex_valid(mutex)        sys_sem_valid(mutex)
#define sys_mutex_set_invalid(mutex)  sys_sem_set_invalid(mutex)
#endif

#if 0
/* XXX because ucos mutex has priority inherite,
 * i cann't figure out OSMutexCreate(prio, &error),which prio is 
 * appropriate, certainly each mutex must have different prio when 
 * created.
 */

/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
	u8_t error;
	OS_EVENT *p;

	/* notes:
	 * create a mutex only higher than the idle process,
	 * other process must not use priority (OS_LOWEST_PRIO - 1),
	 * or it will cause OSMutexCreate return error OS_PRIO_EXIST
	 */
	p = OSMutexCreate(OS_LOWEST_PRIO - 1, &error);
	if(error == OS_NO_ERR) {
		mutex->event = p;
		mutex->valid = 1;
		return ERR_OK;
	} else if (error == OS_PRIO_EXIST) {
		/* see above notes */
		RERR();
	} else {
		RERR();
	}

	return -ENOMEM;
}

/** Lock a mutex
 * @param mutex the mutex to lock */
void sys_mutex_lock(sys_mutex_t *mutex)
{
	u8_t error;
	OSMutexPend(mutex->event, 0, &error);
	if(error != OS_NO_ERR) {
		RERR();
	}
}


/** Unlock a mutex
 * @param mutex the mutex to unlock */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	u8_t error;
	error = OSMutexPost(mutex->event);
	if(error != OS_NO_ERR) {
		RERR();
	}
}

/** Delete a semaphore
 * @param mutex the mutex to delete */
void sys_mutex_free(sys_mutex_t *mutex)
{
	u8_t error;
	OSSchedLock();
	mutex->valid = 0;
	OSMutexDel(mutex->event, OS_DEL_ALWAYS, &error);
	if(error != OS_NO_ERR) {
		RERR();
	}
	OSSchedUnlock();
}

#ifndef sys_mutex_valid
/** Check if a mutex is valid/allocated: return 1 for valid, 0 for invalid */
int sys_mutex_valid(sys_mutex_t *mutex)
{
	return mutex->valid;
}


#endif
#ifndef sys_mutex_set_invalid
/** Set a mutex invalid so that sys_mutex_valid returns 0 */
void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
	mutex->valid = 0;
}
#endif
#endif


/* XXX this may cause potential problem,lwip require sys_msleep() has
 * 1ms resolution, but currently we offer 10ms resolution
 */
/* only has a (close to) 1 jiffy resolution. */
void sys_sleep_ms(u32_t ms) 
{
	unsigned int tick;

	tick = (ms + SYS_TIMER_INTERVAL)/SYS_TIMER_INTERVAL;
	OSTimeDly(tick);
}


/** The only thread function:
 * Creates a new thread
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	u8_t error;
	sys_thread_t p;
	void *task_stack;
	int size;

	size = sizeof(*p) + stacksize + 7;
	p = malloc(size);
	if(p == NULL)
		return NULL;

	/* align stack, arm require stack align at 8 bytes */
	task_stack = (unsigned int *)(((unsigned int)p + size) & ~0x07);
	
	error = OSTaskCreate(thread, arg, task_stack,prio); 
	if(error != OS_NO_ERR) {
		RERR();
		return NULL;
	}

	p->thread = thread;
	p->prio = prio;
	strcpy(p->name, name);
	return p;
}

sys_thread_t sys_thread_del(sys_thread_t thread)
{
	/* XXX need this ? */

	RERR();
	return NULL;
}

/* sys_init() must be called before anthing else. */
void sys_init(void)
{

}

#ifndef sys_jiffies
/** Ticks/jiffies since power up. */
u32_t sys_jiffies(void)
{
	return OSTimeGet();
}
#endif

/** Returns the current time in milliseconds,
 * may be the same as sys_jiffies or at least based on it. */
u32_t sys_now(void)
{
	return OSTimeGet() * SYS_TIMER_INTERVAL;
}


unsigned int sys_arch_protect(void)
{
	return irq_save_asm();
}

void sys_arch_unprotect(unsigned long lev)
{
	irq_restore_asm(lev);
}

unsigned int CurrentThreadPrio(void)
{
	return (unsigned int)OSPrioCur;
}

