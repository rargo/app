#include <base.h>
#include <timer.h>
#include <irq.h>
#include <eint.h>
#include <lwip/sockets.h>
#include <lwip/ppp.h>
#include <ucos.h>
#include <uart.h>
#include <wireless.h>
#include <nand.h>

void ucos_test(void);
int eth_init(void);
void s_initNet(void);
void lwip_dm9000_init(void);
void lwip_test(void);
void sys_timer_start(void);

static unsigned int thread_stack[64*1024];//256KB
void ethernet_test_thread(void *arg)
{
	while(1) {
		RDIAG(LEGACY_DEBUG,"main_thread run");
		lwip_test();
		OSTaskSuspend(OS_PRIO_SELF);
	}
}

/* if ppp link status change, function will be call to handle 
 * serial port
 */
void pppLinkStatusCallBack(void *ctx, int errCode, void *arg)
{
	RDIAG(LEGACY_DEBUG,"pppLinkStatusCallBack errCode:%d",errCode);

	switch(errCode) 
	{


	}
}

void ppp_test_thread(void *arg)
{
	sio_fd_t sio_fd;
	int ppp_fd;
	int count;
	int pppif_up;
	int ret;
	unsigned int c1,c2;

	wl_init();
	wl_connect();

	sio_fd = sio_open(3);
	pppInit();
	c1 = cpu_counter();
	//RDIAG(LEGACY_DEBUG,"c1:0x%8x",cpu_counter());
	ppp_fd = pppOverSerialOpen(sio_fd, pppLinkStatusCallBack, NULL);
	RASSERT(ppp_fd >= 0);
	count = 0;
	RDIAG(LEGACY_DEBUG,"check ppp up");
	while(1) {
		ret = pppIOCtl(ppp_fd, PPPCTLG_UPSTATUS, &pppif_up);
		RASSERT(ret == 0);
		if(pppif_up) {
			c2 = cpu_counter();
			RDIAG(LEGACY_DEBUG,"c1:0x%8x, c2:0x%8x", c1, c2);
			RDIAG(LEGACY_DEBUG,"timer cost:%d", cpu_counter_ns(c2,c1));
			RDIAG(LEGACY_DEBUG,"ppp if up now");
			break;
		}
		OSTimeDly(100); /* check ppp up every 1S */
		if(count++ >= 20) {
			RERR("wait ppp up timeout!");
			break;
		}
		c2 = cpu_counter();
		RDIAG(LEGACY_DEBUG,"timer cost:%dns", cpu_counter_ns(c2,c1));
		c1 = c2;
		RDIAG(LEGACY_DEBUG,"%dS",count);
	}

	lwip_test();
}

#if 0
void nand_test(void)
{
	int i;
	int ret;
	unsigned char blk_buf[256*1024];
	unsigned char page_buf[2048];
#define TEST_BLOCK 7168
	nand_init();
	//nand_bbt_scan();
#if 0
	nand_read_page(0, buf, sizeof(buf));
	dlog_hex(buf,sizeof(buf));
#endif
#if 0
	ret = nand_erase_block(TEST_BLOCK);
	RDIAG(NAND_DEBUG,"nand_erase_block ret:%d",ret);
#endif
	ret = nand_read_raw(nand_block_to_addr(TEST_BLOCK),blk_buf,nand_block_size());
	RDIAG(NAND_DEBUG,"nand_read_raw ret:%d",ret);
	for(i=0;i<nand_block_size();i++) {
		if(blk_buf[i] != 0xff) {
			RERR("nand erase result error");
			break;
		}
	}

	for(i=0;i<nand_page_size();i++) {
		page_buf[i] = i & 0xff;
	}

	ret = nand_write_page(nand_addr_to_page(nand_block_to_addr(TEST_BLOCK)),page_buf, nand_page_size());
	RDIAG(NAND_DEBUG,"ret:%d",ret);
	ret = nand_read_page(nand_addr_to_page(nand_block_to_addr(TEST_BLOCK)),page_buf, nand_page_size());
	dlog_hex(page_buf,nand_page_size());
	for(i=0;i<nand_page_size();i++) {
		if(page_buf[i] != (i & 0xff)) {
			RERR("nand_write_page result error");
			break;
		}
	}

	//dlog_hex(blk_buf, nand_block_size());

	while(1)
		;
}
#endif

void test_tasks(void *arg)
{


}

int main(void)
{
	char sp;
	int error;

	softirq_init();
	serial_init();
	dprintf("main &sp:0x%8x\r\n",&sp);
	dprintf("main start, compile date:%s %s\r\n", __DATE__, __TIME__);

	cpu_counter_init();
	irq_init();
	uart_init();
	eint_init(3, 5, 6, 7);
	/*10ms internal */
	sys_timer_init(10); 
	/* start timer service */
	sys_timer_start(); 
	event_handler_t sys_tick_h = HANDLER(sys_tick,NULL);
	sys_timer_attach_handler(&sys_tick_h);

	//nand_test();

#if 1
	/* attach ucos timer handler to sys timer */
	event_handler_t ucos_tick_h = HANDLER(OSTimeTick,NULL);
	sys_timer_attach_handler(&ucos_tick_h);
	/* ucos init */
	OSInit();
	/* lwip initial */
	tcpip_init(NULL, NULL);
	/* add eth(dm9000) interface,initial dm9000 */
	lwip_dm9000_init();
	//OSTimeDly(100);
	error = OSTaskCreate(ethernet_test_thread, NULL, (OS_STK *)&thread_stack[64*1024], 15); //prio: 30
	//error = OSTaskCreate(ppp_test_thread, NULL, (OS_STK *)&thread_stack[64*1024], 30); //prio: 30
	if(error != OS_NO_ERR)
		RERR();
	OSStart();
	RERR();
#endif

	return 1;
}

#define MAX_LOOP_TASK 32
#define LOOP_TASK_PRIO 30
#define LOOP_TASK_STACK_SIZE 64*1024
static unsigned int loop_task_stack[MAX_LOOP_TASK][LOOP_TASK_STACK_SIZE/4];

void loop_task(void *arg)
{
	int ret;
	unsigned char buf[1500];
	int loop_socket = *(int *)arg;
	while(1) {
		ret = lwip_recv(loop_socket, buf, sizeof(buf) - 1, 0);
		RDIAG(LEGACY_DEBUG,"lwip recv ret:%d",ret);

		if(ret > 0) {
			lwip_send(loop_socket, buf, ret, 0);
			buf[ret] = 0;
			RDIAG(LEGACY_DEBUG,"lwip recv %d bytes",ret);
			//RDIAG(LEGACY_DEBUG,"lwip recv: %s\r\n",buf);
		} else {
#if 1
			RDIAG(LEGACY_DEBUG,"lwip close socket %d",loop_socket);
			lwip_close(loop_socket);
			RDIAG(LEGACY_DEBUG,"suspend current task");
			OSTaskSuspend(OS_PRIO_SELF);
#if 0
			RDIAG(LEGACY_DEBUG,"!!lwip recv ret:%d, delete current task",ret);
			OSTaskDel(OS_PRIO_SELF);
#endif
#endif
			break;
		}
		OSTimeDly(3);
	}
}
		

#if 0
	lwip_bind(int s, const struct sockaddr *name, socklen_t namelen)
	lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
lwip_recv(int s, void *mem, size_t len, int flags)
#endif
#define IP4ADDR_PORT_TO_SOCKADDR(sin, ipXaddr, port) do { \
	(sin)->sin_len = sizeof(struct sockaddr_in); \
	(sin)->sin_family = AF_INET; \
	(sin)->sin_port = htons((port)); \
	(sin)->sin_addr.s_addr = htons((ipXaddr)); \
	memset((sin)->sin_zero, 0, SIN_ZERO_LEN); }while(0)
void lwip_test(void)
{
	int s;
	int ret;
	struct sockaddr_in test_ip;
	ip_addr_t ipaddr; 
	int s_new;
	socklen_t addrlen;
	struct sockaddr addr;
	unsigned char buf[1500];
	int cur_task;
	int loop_task_prio;
	unsigned char error;

#if 0
	int i;
	for(i=0; i<50000; i++)
	{
		//dprintf("malloc test:%d\r\n",i);
		p_test_ip = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
		memset(p_test_ip, 0xee, sizeof(struct sockaddr_in));

		free(p_test_ip);
	}
	dprintf("malloc test:%d\r\n",i);
#endif

	//IP4_ADDR(&ipaddr, 58,27,86,120); /* www.bing.com */

	test_ip.sin_len = sizeof(struct sockaddr_in);
	test_ip.sin_family = AF_INET;
	//test_ip.sin_port = htons((21));
	test_ip.sin_port = htons((80));
	IP4_ADDR(&ipaddr, 192,168,1,100);
	inet_addr_from_ipaddr(&test_ip.sin_addr, &ipaddr);
	memset(test_ip.sin_zero, 0, SIN_ZERO_LEN);

	s = lwip_socket(AF_INET, SOCK_STREAM, 0);

	ret = lwip_bind(s, &test_ip, sizeof(test_ip));
	RDIAG(LEGACY_DEBUG,"lwip_bind ret:%d\r\n",ret);

	ret = lwip_listen(s, 5);//max listen connection:5
	RDIAG(LEGACY_DEBUG,"lwip_listen ret:%d\r\n",ret);

	cur_task = 0;
	loop_task_prio = LOOP_TASK_PRIO;
	while(1) {
		s_new = lwip_accept(s, &addr, &addrlen);
		RDIAG(LEGACY_DEBUG,"lwip_accept ret============================:%d\r\n",s_new);

		if(s_new <= 0)
			RERR();

#if 0
		while(1) {
			ret = lwip_recv(s_new, buf, sizeof(buf) - 1, 0);

			if(ret > 0) {
				lwip_send(s_new, buf, ret, 0);
				buf[ret] = 0;
				RDIAG(LEGACY_DEBUG,"lwip recv %d bytes",ret);
				//RDIAG(LEGACY_DEBUG,"lwip recv: %s\r\n",buf);
			} else {
				RDIAG(LEGACY_DEBUG,"!!!!!!!lwip recv ret:%d\r\n",ret);
				lwip_close(s_new);
				s_new = -1;
				//OSTaskSuspend(OS_PRIO_SELF);
				break;
			}
		}
#else
		if(cur_task >= MAX_LOOP_TASK) {
			RERR("MAX_TASK is %d",MAX_LOOP_TASK);
			lwip_close(s_new);
			lwip_close(s);
			break;
		}
		error = OSTaskCreate(loop_task, &s_new, (OS_STK *)&loop_task_stack[cur_task+1][0], loop_task_prio); //prio: 30
		if(error != OS_NO_ERR)
			RERR("err:%2x",error);
		else {
			cur_task++;
			loop_task_prio++;
		}
#endif
	}

	OSTaskSuspend(OS_PRIO_SELF);
}

