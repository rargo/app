#ifndef BASE_H
#define BASE_H

#include <s3c6410.h>
#include <linux/errno.h>
#include <string.h>
#include <stdarg.h>
#include <log.h>

int sprintf(char * buf, const char *fmt, ...);
int snprintf(char *buf, int len ,const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

#define readb(a)			(*(volatile unsigned char *)(a))
#define readw(a)			(*(volatile unsigned short *)(a))
#define readl(a)			(*(volatile unsigned int *)(a))

#define writeb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define writew(v,a)		(*(volatile unsigned short *)(a) = (v))
#define writel(v,a)		(*(volatile unsigned int *)(a) = (v))

#define setbitb(a,bit) (*(volatile unsigned char *)(a) |= 1UL << (bit))
#define setbitw(a,bit) (*(volatile unsigned short *)(a) |= 1UL << (bit))
#define setbitl(a,bit) (*(volatile unsigned int *)(a) |= 1UL << (bit))

#define clrbitb(a,bit) (*(volatile unsigned char *)(a) &= ~(1UL << (bit)))
#define clrbitw(a,bit) (*(volatile unsigned short *)(a) &= ~(1UL << (bit)))
#define clrbitl(a,bit) (*(volatile unsigned int *)(a) &= ~(1UL << (bit)))

#define ARRAY_SIZE(array) (sizeof(array)/array[0])

extern unsigned int fclk;
extern unsigned int hclk;
extern unsigned int pclk;
extern int errno;

#define MALLOC_MEM_SIZE 32*1024*1024 // 32M

/* memory map:
 * 0xc0000000-0xc8000000: use by the system.
 *
 * -----------  STACK_TOP:0xc8000000
 * |ABT_STACK|	@64K
 * -----------
 * |UND_STACK|	@64K
 * -----------
 * |FIQ_STACK|	@64K
 * -----------
 * |IRQ_STACK|	@512K
 * -----------
 * |SVC_STACK|	@32M|system run in svc mode
 * -----------
 * |USR_STACK|	@XXX not used here
 * -----------
 *
 *
 * -----------
 * |  32M    |	for malloc() use
 * -----------	<---- &end
 * |.bss COMMON|  
 * -----------
 * |.data    |
 * -----------
 * |.text    |
 * ----------- _start:0xc0000000
 *
 */

#define SYS_TIMER_HZ 100 /* system timer frequency */
#define SYS_TIMER_INTERVAL (1000/SYS_TIMER_HZ) /* system timer interval in ms */

extern unsigned int irq_save_asm(void);
extern void irq_restore_asm(unsigned int cpu_sr);

#define local_irq_save(cpu_irq_status) do{cpu_irq_status = irq_save_asm();}while(0)
#define local_irq_restore(cpu_irq_status) irq_restore_asm(cpu_irq_status)

#define EINT(n) (n)
#define EINT_NUM 28

void dprintf(char *fmt, ...);

void* malloc(size_t bytes);
void free(void *mem);

#define printf dprintf

#define ROS_DEBUG_ON 0x80
#define ROS_DEBUG_OFF 0x00

/* global debug control */
#define ROS_DEBUG_TYPES_ON ROS_DEBUG_ON

/* debug types */
#define MBOX_DEBUG ROS_DEBUG_ON
//#define MBOX_DEBUG ROS_DEBUG_OFF

#define ETH_DEBUG ROS_DEBUG_ON
//#define ETH_DEBUG ROS_DEBUG_OFF

#define UCOS_DEBUG ROS_DEBUG_ON
//#define UCOS_DEBUG ROS_DEBUG_OFF

#define LEGACY_DEBUG ROS_DEBUG_ON

#define TASK_SW_DEBUG ROS_DEBUG_ON
//#define TASK_SW_DEBUG ROS_DEBUG_OFF
//
#define SOFTIRQ_DEBUG ROS_DEBUG_ON
//#define SOFTIRQ_DEBUG ROS_DEBUG_OFF
//
#define NAND_DEBUG ROS_DEBUG_ON

/* RDEBUG controls RDIAG, RERR */
#define RDIAG(debug, ...) do{ \
			if(((debug) & ROS_DEBUG_ON) && \
				((debug) & ROS_DEBUG_TYPES_ON)) {\
				dlog("[DIAG %s, %5d]:", __func__, __LINE__); \
				dlog("" __VA_ARGS__); \
				dlog("\r\n"); \
			} \
		}while(0)

#define RERR(format, ...) do{ \
							dlog("[ERR %s, %5d]:", __func__, __LINE__);\
							dlog(format "\r\n", ## __VA_ARGS__ );}while(0)

/* infinite loop */
#define RASSERT(expr) do{ if(!(expr)) { \
							unsigned int irq_status;\
							local_irq_save(irq_status);\
							dlog("[ASSERT FAIL %s, %s, %d]!\r\n",\
									__FILE__,__func__,__LINE__);\
							for(;;) \
								;\
						} \
					}while(0)

void cpu_counter_init(void);
unsigned int cpu_counter(void);
/* in s,assume cpu counter not overflow 
 * in 532M, max is 8.07S
 * */
//unsigned int cpu_counter_time(unsigned int stop, unsigned int start);

typedef struct event_handler {
	void (*fn)(void *arg);
	void *arg;
} event_handler_t;


#define HANDLER(fn,arg)  {(void (*)(void *))fn, (void *)arg}

void sys_tick(void);

void debug_s_irq_handler_in(unsigned int *stack);
void debug_s_irq_handler_out(unsigned int *stack);

unsigned int cpu_counter_ns(unsigned int stop, unsigned int start);
void cpu_counter_init(void);
unsigned int cpu_counter(void);

void softirq_init(void);
int softirq_require(void (*h)(void));
int softirq_raise(int n);

#define ARRAY_SIZE(array) (sizeof(array)/array[0])

/* ucos task prio assign */


#define TASK_PRIO_LWIP_PPP_THREAD_PRIO                 7
#define TASK_PRIO_LWIP_TCPIP_THREAD_PRIO               8
#define TASK_PRIO_LWIP_DEFAULT_THREAD_PRIO             10

#define TASK_PRIO_MAIN 30

#endif
