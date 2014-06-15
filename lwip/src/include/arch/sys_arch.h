#ifndef SYS_ARCH_H
#define SYS_ARCH_H

#include "lwip/arch.h"
#include "lwip/err.h"
#include <ucos.h>
//#include <base.h>

/** Return code for timeouts from sys_arch_mbox_fetch and sys_arch_sem_wait */
#define SYS_ARCH_TIMEOUT 0xffffffffUL

typedef void (*lwip_thread_fn)(void *arg);

typedef struct sys_mbox_s {
	int valid;
	OS_EVENT *event;

	/* pointer to the malloc area start,
	 * use for mbox_free.
	 */
	void *malloc_start; 
} sys_mbox_t;
void sys_mbox_post(sys_mbox_t *mbox, void *msg);
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg);
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout);
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg);
void sys_mbox_free(sys_mbox_t *mbox);
int sys_mbox_valid(sys_mbox_t *mbox);
void sys_mbox_set_invalid(sys_mbox_t *mbox);

typedef struct sys_sem_s {
	int valid;
	OS_EVENT *event;
} sys_sem_t;
err_t sys_sem_new(sys_sem_t *sem, u8_t count);
void sys_sem_signal(sys_sem_t *sem);
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout);
void sys_sem_free(sys_sem_t *sem);
void sys_sem_wait(sys_sem_t *sem);
int sys_sem_valid(sys_sem_t *sem);
void sys_sem_set_invalid(sys_sem_t *sem);

/* XXX use semaphore to implement mutex, please see comments 
 * in sys_arch.c why don't use ucos's own mutex implement.
 */
#define LWIP_COMPAT_MUTEX 1
typedef struct sys_mutex_s {
	int valid;
	OS_EVENT *event;
} sys_mutex_t;

typedef struct sys_thread_s {
	lwip_thread_fn thread;
	void *arg;
	char name[32];
	int prio;
} *sys_thread_t;

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio);
sys_thread_t sys_thread_del(sys_thread_t thread);
void sys_sleep_ms(u32_t ms);
void sys_init(void);
u32_t sys_jiffies(void);
u32_t sys_now(void);
unsigned int sys_arch_protect(void);
void sys_arch_unprotect(unsigned long lev);
unsigned int CurrentThreadPrio(void);

#endif
