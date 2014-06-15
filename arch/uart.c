#include <s3c6410.h>
#include <stdarg.h>
#include <base.h>
#include <irq.h>
#include <arch/sys_arch.h>

#define UART_REG_W(ch, reg_offset, value) *(volatile unsigned int *)\
	(0x7f005000 + 0x400*ch + reg_offset) = (value)

#define UART_REG_R(ch, reg_offset) (*(volatile unsigned int *)\
	(0x7f005000 + 0x400*ch + reg_offset))

extern int vsprintf(char *buf, const char *fmt, va_list args);

/* recommand by samsung */
static const unsigned int divslot[] = { 
	0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x4924, 0x4a52, 0x54aa, 
	0x5555, 0xd555, 0xd5d5, 0xddd5, 0xdddd, 0xdfdd, 0xdfdf, 0xffdf
};

/* configure uart0:
 * 115200,8,n,1 
 * non fifo mode
 * no interrupt is enable
 */
int serial_init(void)
{
	int ch = 0;
	unsigned int baudrate = 115200;
	char parity = 'n';
	int databits = 8;
	int stopbits = 1;

	unsigned int reg;
	double ddiv;
	unsigned int div;
	unsigned int num_one;

	reg = 0; //normal mode
	switch(parity)
	{
		case 'n': /* no parity */
			break;
		case 'o': /* odd */
			reg |= 4UL<<3;
			break;
		case 'e': /* even */
			reg |= 5UL<<3;
			break;
		default:
			return -EINVAL;
	}
	switch(databits)
	{
		case 5:
		case 6:
		case 7:
		case 8:
			reg |= databits - 5;
			break;
		default:
			return -EINVAL;
	}
	switch(stopbits)
	{
		case 1:
		case 2:
			reg |= (stopbits - 1) << 2;
			break;
		default:
			return -EINVAL;
	}

	irq_mask(IRQ_UART0 + ch);

	UART_REG_W(ch, ULCON_OFFSET, reg);

	/*
	 * pclk: tx level interrupt : rx level interrupt
	 * : enable rx-reiceve timeout : generate rx error interrupt
	 * : normal operation(not loopback mode)
	 * : normal transmit(not send break signal)
	 * : tx interrupt or polling mode(not dma mode)
	 * : rx interrupt or polling mode(not dma mode)
	 * 10 1 1 1 1 0 0 01 01 
	 */
	UART_REG_W(ch, UCON_OFFSET, 0xbc5);

	 /* fifo is disable:
	  */
	/* tx fifo 0bytes : rx fifo 16bytes : : clear tx fifo : clear rx fifo \
	 * : fifo disable
	 * 00 10 0 1 1 0
	 */
	UART_REG_W(ch, UFCON_OFFSET, 0x26);

	ddiv = ((double)pclk / (baudrate * 16)) - 1;
	div = (unsigned int)ddiv;
	num_one = (unsigned int)((ddiv - div) * 16);
	UART_REG_W(ch, UBRDIV_OFFSET, div);
	UART_REG_W(ch, UDIVSLOT_OFFSET, divslot[num_one]);

	/* clear source pending interrupt flag */
	UART_REG_W(ch,UINTSP_OFFSET, 0x0f);
	
	/* disable all uart interrupt */
	UART_REG_W(ch, UINTM_OFFSET, 0x0f);
	/* clear pending interrupt flag */
	UART_REG_W(ch,UINTP_OFFSET, 0x0f);

	return 0;
}

char serial_getc(void)
{
	while(!(UTRSTAT0_REG & 0x01))
		;

	return URXH0_REG & 0xff;
}

void serial_putc(const char c)
{
	while(!(UTRSTAT0_REG & 0x02))
		;

	UTXH0_REG = c;
}

void serial_puts(const char *s)
{
	while(*s)
		serial_putc(*s++);
}

void serial_send(const char *s, int len)
{
	while(len--)
		serial_putc(*s++);
}

static char serial_printf_buffer[1024];
void dprintf(char *fmt, ...)
{
	va_list varg;

	//memset(serial_printf_buffer, 0, sizeof(serial_printf_buffer));
	va_start(varg, fmt);
	vsprintf(serial_printf_buffer, fmt, varg);
	va_end(varg);

	serial_puts(serial_printf_buffer);
}


/* uart 1~3 channel use follow api, intterupt is used */

#define MAX_CHANNEL 4
#define TX_BUF 8093 /* real tx buffer size is TX_BUF - 1 */
#define RX_BUF 8093 /* real rx buffer size is RX_BUF - 1 */
typedef struct uart_s {
	int is_open; /* 0: close,1: open */
	int tx_buf[TX_BUF];
	int rx_buf[RX_BUF];
	int tx_w;
	volatile int tx_r;
	volatile int rx_w;
	int rx_r;

	sys_sem_t sem_rx_empty; /*task sleep on rx empty*/
	int sem_rx_empty_valid;
	sys_sem_t sem_tx_full;/*task sleep on tx full */
	int sem_tx_full_valid;
} uart_t;

static uart_t uart[MAX_CHANNEL];

/* enable interrupts:
 *	tx: when fifo iss less than triger level
 *	rx: 
 *		1. when fifo is large than triger level
 *		2. receive timeout and fifo is not empty
 *	error status:
 *		rx receive error.
 */
static void uart_isr(int ch)
{
	unsigned int status;
	unsigned int mask;
	unsigned int ufstat;
	int call_rx_buffer_event = 0;
	int call_tx_buffer_event = 0;

	status = UART_REG_R(ch, UINTP_OFFSET);
	mask = UART_REG_R(ch, UINTM_OFFSET);
	/* mask this channel's all interrupt */
	UART_REG_W(ch, UINTM_OFFSET, 0x0f);
	/* clear uart interrupt */
	UART_REG_W(ch, UINTSP_OFFSET, status);
	UART_REG_W(ch, UINTP_OFFSET, status);

	//RDIAG(LEGACY_DEBUG,"uart ch:%d isr",ch);

	/* rx interrupt:
	 *	1. when fifo is large than triger level
	 *	2. receive timeout and fifo is not empty
	 */
	if(status & 0x01) {
		unsigned int rx_count;

		ufstat = UART_REG_R(ch, UFSTAT_OFFSET) & 0xff;

		rx_count = ufstat & 0x7f;

		/* if rx buffer was empty before, and we receive some ch
		 * rx_buffer_event should be called
		 */
		if(rx_count && (uart[ch].rx_w == uart[ch].rx_r))
			call_rx_buffer_event = 1;

		//RDIAG(LEGACY_DEBUG,"rx %d bytes",rx_count);

		/* read to rx buf */
		while(rx_count--) {
			if((uart[ch].rx_w + 1) % RX_BUF == uart[ch].rx_r) {
				/* bad, rx_buf overflow, don't fill any more char into it */
				RERR("uart:%d rx overflow, data may lost!", ch);
				break;
			}
			uart[ch].rx_buf[uart[ch].rx_w] = UART_REG_R(ch, URXH_OFFSET);
			uart[ch].rx_w = (uart[ch].rx_w + 1) % RX_BUF;
		}
	}

	/* rx error status interrupt */
	if(status & 0x02) {
		unsigned int error_status;
		RDIAG(LEGACY_DEBUG,"rx error!!!");
		/* read to clear error status register */
		error_status = UART_REG_R(ch, UERSTAT_OFFSET);
	}

	/* tx interrupt */
	if(status & 0x04) {

		/* if before uart tx buffer is full, tx_buffer_event will be called */
		if((uart[ch].tx_w + 1) % TX_BUF == uart[ch].tx_r)
			call_tx_buffer_event = 1;

		//RDIAG(LEGACY_DEBUG,"tx fifo empty");
		/* fill tx fifo until it is full */
		while((UART_REG_R(ch, UFSTAT_OFFSET) & (1<<14)) == 0) {
			if(uart[ch].tx_r == uart[ch].tx_w) {
				/* tx buf now is empty, disable tx interrupt */
				mask |= 1<<2; 
				break;
			}
			UART_REG_W(ch,UTXH_OFFSET, uart[ch].tx_buf[uart[ch].tx_r]);
			uart[ch].tx_r = (uart[ch].tx_r + 1) % TX_BUF;
		}
	}

	UART_REG_W(ch, UINTM_OFFSET, mask);

	/* call the event handler,usually wakeup some tasks handler
	 * uart data.
	 */
	if(call_tx_buffer_event && uart[ch].sem_tx_full_valid) {
		uart[ch].sem_tx_full_valid = 0; /* one time signal */
		sys_sem_signal(&uart[ch].sem_tx_full);
	}
	if(call_rx_buffer_event && uart[ch].sem_rx_empty_valid) {
		uart[ch].sem_rx_empty_valid = 0; /* one time signal */
		sys_sem_signal(&uart[ch].sem_rx_empty);
	}
}

static void uart1_isr(void)
{
	uart_isr(1);
}

static void uart2_isr(void)
{
	uart_isr(2);
}

static void uart3_isr(void)
{
	uart_isr(3);
}

/* init uart1~3(uart0 is for terminal use) */
void uart_init(void)
{

}

/* baudrate:
 * parity:
 *		0: no parity
 *		1: odd
 *		2: even
 *	databits:
 *		6~8
 *	stopbits:
 */


int uart_open(int ch, unsigned int baudrate,unsigned int databits, char parity,unsigned int stopbits)
{
	unsigned int reg;
	double ddiv;
	unsigned int div;
	unsigned int num_one;

	if(ch == 0 || ch > 3) {
		/* channel 0 is use for terminal */
		return -EINVAL;
	}

	if(uart[ch].is_open != 0)
		return -EBUSY;

	if(baudrate < 1200 || baudrate > 256000)
		return -EINVAL;

	reg = 0; //normal mode
	switch(parity)
	{
		case 'n': /* no parity */
			break;
		case 'o': /* odd */
			reg |= 4UL<<3;
			break;
		case 'e': /* even */
			reg |= 5UL<<3;
			break;
		default:
			return -EINVAL;
	}
	switch(databits)
	{
		case 5:
		case 6:
		case 7:
		case 8:
			reg |= databits - 5;
			break;
		default:
			return -EINVAL;
	}
	switch(stopbits)
	{
		case 1:
		case 2:
			reg |= (stopbits - 1) << 2;
			break;
		default:
			return -EINVAL;
	}

	irq_mask(IRQ_UART0 + ch);

	UART_REG_W(ch, ULCON_OFFSET, reg);

	/*
	 * pclk: tx level interrupt : rx level interrupt
	 * : enable rx-reiceve timeout : generate rx error interrupt
	 * : normal operation(not loopback mode)
	 * : normal transmit(not send break signal)
	 * : tx interrupt or polling mode(not dma mode)
	 * : rx interrupt or polling mode(not dma mode)
	 * 10 1 1 1 1 0 0 01 01 
	 */
	UART_REG_W(ch, UCON_OFFSET, 0xbc5);

	 /* fifo is enable:
	  * tx interrupt will be generate if fifo less than or equals setting level,
	  * rx interrupt will be gererate if fifo reach setting level.
	  */
	/* tx fifo 16bytes : rx fifo 16bytes : : clear tx fifo : clear rx fifo \
	 * : fifo enable
	 * 01 10 0 1 1 1
	 */
	UART_REG_W(ch, UFCON_OFFSET, 0x27);

	/* set baudrate */
	ddiv = ((double)pclk / (baudrate * 16)) - 1;
	div = (unsigned int)ddiv;
	num_one = (unsigned int)((ddiv - div) * 16);
	UART_REG_W(ch, UBRDIV_OFFSET, div);
	UART_REG_W(ch, UDIVSLOT_OFFSET, divslot[num_one]);

	/* clear tx,rx buffer */
	uart[ch].tx_w = 0;
	uart[ch].tx_r = 0;
	uart[ch].rx_w = 0;
	uart[ch].rx_r = 0;

	sys_sem_new(&uart[ch].sem_rx_empty,0);
	sys_sem_new(&uart[ch].sem_tx_full,0);

	/* uart1~3 prio: 8 */
	switch(ch) {
		case 1:
			irq_register_handler(IRQ_UART1, 8, uart1_isr);
			break;
		case 2:
			irq_register_handler(IRQ_UART2, 8, uart2_isr);
			break;
		case 3:
			irq_register_handler(IRQ_UART3, 8, uart3_isr);
			break;
	}

	/* configure uart gpio */
	switch(ch) {
		case 1: /* GPA4:RX, GPA5:TX */
			GPACON_REG = (GPACON_REG & ~(0xffUL << 16)) | (0x22UL << 16);
			break;
		case 2: /* GPB0:RX, GPB1: TX */
			GPBCON_REG = (GPBCON_REG & ~(0xffUL << 0)) | (0x22UL << 0);
			break;
		case 3: /* GPB2:RX, GPB3: TX */
			GPBCON_REG = (GPBCON_REG & ~(0xffUL << 8)) | (0x22UL << 8);
			break;
	}

	/* clear source pending interrupt flag */
	UART_REG_W(ch,UINTSP_OFFSET, 0x0f);
	
	/* enable rx, error status interrupt */
	UART_REG_W(ch, UINTM_OFFSET, 0x0c);
	/* clear pending interrupt flag */
	UART_REG_W(ch,UINTP_OFFSET, 0x0f);
	
	/* channel is open now */
	uart[ch].is_open = 1;

	irq_unmask(IRQ_UART0 + ch);

	return 0;
}


/* return left tx bytes */
int uart_tx(int ch, const char *buf, int len)
{
	unsigned int mask;

	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}

	while(len > 0) {
		if((uart[ch].tx_w + 1) % TX_BUF == uart[ch].tx_r) {
			/* bad, tx_buf is full */
			RERR("uart:%d, tx_buf is full, cann't fill any more bytes!",ch);
			break;
		}
		uart[ch].tx_buf[uart[ch].tx_w] = *buf++;
		uart[ch].tx_w = (uart[ch].tx_w + 1) % TX_BUF;
		len--;
	}

	irq_mask(IRQ_UART0 + ch);
	mask = UART_REG_R(ch, UINTM_OFFSET);
	mask &= ~(1<<2); /* enable tx interrupt */
	UART_REG_W(ch, UINTM_OFFSET, mask);
	irq_unmask(IRQ_UART0 + ch);

	return len;
}

int uart_tx_str(int ch, const char *str)
{
	return uart_tx(ch, (const char *)str, strlen(str));
}

static char uart_printf_buffer[2048];
int uart_printf(int ch, const char *fmt, ...)
{
	va_list varg;

	va_start(varg, fmt);
	vsprintf((char *)uart_printf_buffer, fmt, varg);
	va_end(varg);

	return uart_tx(ch, uart_printf_buffer, \
			strlen((char *)uart_printf_buffer));
}

/* return receive bytes */
int uart_rx(int ch, char *buf, int len)
{
	int receive = 0;
	
	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}
	
	while(len > 0) {
		if(uart[ch].rx_r == uart[ch].rx_w) {
			/* rx buf is empty now */
			break;
		}
		*buf++ = uart[ch].rx_buf[uart[ch].rx_r];
		uart[ch].rx_r = (uart[ch].rx_r + 1) % RX_BUF;
		receive++;
		len--;
	}

	return receive;
}

/* return 1 if all charactor in tx buffer has been successfully sent */
int uart_tx_finish(int ch)
{
	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}

	/* tx finish means:
	 * tx buffer empty, tx fifo empty, and transmit holding and shift register empty.
	 */
	if(uart[ch].tx_r == uart[ch].tx_w \
		&& (UART_REG_R(ch, UFSTAT_OFFSET) & 0x7f00) == 0 \
		&& (UART_REG_R(ch, UTRSTAT_OFFSET) & 0x04) == 0x04)
		return 1;
	else
		return 0;
}

/* return 1 if tx buffer is empty */
int uart_tx_buffer_empty(int ch)
{
	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}

	return uart[ch].tx_r == uart[ch].tx_w;
}

/* return 1 if tx buffer is full */
int uart_tx_buffer_full(int ch)
{
	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}

	if((uart[ch].tx_w + 1) % TX_BUF == uart[ch].tx_r)
		return 1;
	else
		return 0;
}

/* clear uart tx buffer,return 0 when success */
int uart_tx_buffer_clear(int ch)
{
	if(uart[ch].is_open != 1)
		return -ENODEV; 
	irq_mask(IRQ_UART0 + ch);
	uart[ch].tx_r = 0;
	uart[ch].tx_w = 0;
	irq_unmask(IRQ_UART0 + ch);

	return 0;
}

/* clear uart rx buffer,return 0 when success */
int uart_rx_buffer_clear(int ch)
{
	if(uart[ch].is_open != 1)
		return -ENODEV; 
	
	irq_mask(IRQ_UART0 + ch);
	uart[ch].rx_r = 0;
	uart[ch].rx_w = 0;
	irq_unmask(IRQ_UART0 + ch);

	return 0;
}

/* close the ch
 * send and receive buf will be clear
 */
void uart_close(int ch)
{
	if(uart[ch].is_open != 1) {
		RERR();
		return;
	}

	irq_mask(IRQ_UART0 + ch);
	uart[ch].is_open = 0;
	/* disable all uart interrupt */
	UART_REG_W(ch, UINTM_OFFSET, 0x0f);
}

/* return receive bytes */
int uart_rx_sleep(int ch, char *buf, int len)
{
	int receive = 0;
	
	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}
	
	while(len > 0) {
		irq_mask(IRQ_UART0+ch);
		if(uart[ch].rx_r == uart[ch].rx_w) {
			/* rx buf is empty now */
			uart[ch].sem_rx_empty_valid = 1;
			irq_unmask(IRQ_UART0+ch);
			sys_sem_wait(&uart[ch].sem_rx_empty);
			if(uart[ch].rx_r == uart[ch].rx_w) /* we are abort by uart_rx_sleep_abort */
				return receive;
		} else 
			irq_unmask(IRQ_UART0+ch);

		while (len > 0 && uart[ch].rx_r != uart[ch].rx_w) {
			*buf++ = uart[ch].rx_buf[uart[ch].rx_r];
			uart[ch].rx_r = (uart[ch].rx_r + 1) % RX_BUF;
			receive++;
			len--;
		}
	}

	return receive;
}



/* return unsent bytes */
int uart_tx_sleep(int ch, const char *buf, int len)
{
#define ENABLE_SEND() \
	do {\
		if(UART_REG_R(ch, UINTM_OFFSET) & (1<<2)) {\
			/* enable send interrupt */\
			irq_mask(IRQ_UART0 + ch);\
			mask = UART_REG_R(ch, UINTM_OFFSET);\
			mask &= ~(1<<2); /* enable tx interrupt */\
			UART_REG_W(ch, UINTM_OFFSET, mask);\
			irq_unmask(IRQ_UART0 + ch);\
		}\
	} while(0)

	unsigned int mask;

	if(uart[ch].is_open != 1) {
		RERR();
		return -ENODEV;
	}

	while(len > 0) {
		irq_mask(IRQ_UART0+ch);
		if((uart[ch].tx_w + 1) % TX_BUF == uart[ch].tx_r) {
			/* tx_buf is full, wait */
			uart[ch].sem_tx_full_valid = 1;
			irq_unmask(IRQ_UART0+ch);
			ENABLE_SEND();
			sys_sem_wait(&uart[ch].sem_tx_full); 
			if((uart[ch].tx_w + 1) % TX_BUF == uart[ch].tx_r) /* we are abort by uart_tx_sleep_abort */
				return len;
		} else 
			irq_unmask(IRQ_UART0+ch);

		while(len > 0 && (uart[ch].tx_w + 1) % TX_BUF != uart[ch].tx_r) {
			uart[ch].tx_buf[uart[ch].tx_w] = *buf++;
			uart[ch].tx_w = (uart[ch].tx_w + 1) % TX_BUF;
			len--;
		}
	}
	
	ENABLE_SEND();

	return len;

}

void uart_rx_sleep_abort(int ch)
{
	/* don't forbid other process call */
	if(uart[ch].is_open != 1) {
		RERR();
		return;
	}

	irq_mask(IRQ_UART0 + ch);
	if(uart[ch].sem_rx_empty_valid) {
		uart[ch].sem_rx_empty_valid = 0; /* one time signal */
		sys_sem_signal(&uart[ch].sem_rx_empty);
	}
	irq_unmask(IRQ_UART0 + ch);	
}

void uart_tx_sleep_abort(int ch)
{
	/* don't forbid other process call */
	if(uart[ch].is_open != 1) {
		RERR();
		return;
	}

	irq_mask(IRQ_UART0 + ch);
	if(uart[ch].sem_tx_full_valid) {
		uart[ch].sem_tx_full_valid = 0; /* one time signal */
		sys_sem_signal(&uart[ch].sem_tx_full);
	}
	irq_unmask(IRQ_UART0 + ch);	
}

void uart_sleep_abort(int ch)
{
	uart_rx_sleep_abort(ch);
	uart_tx_sleep_abort(ch);
}

void uart_test(void)
{
	char buf[1024];
	char buf2[1024];
	int i;
	int len;
	int ret;

	/* uart1 tx <---> uart3 rx
	 * uart1 rx <---> uart3 tx
	 */
	ret = uart_open(1,115200,8,'n',1);
	dprintf("uart2 open ret:%d\r\n",ret);
	ret = uart_open(3,115200,8,'n',1);
	dprintf("uart3 open ret:%d\r\n",ret);

	while(1) {
		for(i = 0; i<sizeof(buf); i++)
			buf[i] = i & 0xff;
		uart_tx(1, buf, sizeof(buf));

		
		while(!uart_tx_buffer_empty(1)) {
			RDIAG(LEGACY_DEBUG,"wait uart 2 tx finish");
		}

		while(1) {
			len = uart_rx(3, buf2, sizeof(buf2));
			dprintf("len:%d\r\n",len);
			if(len > 0) {
				for(i = 0; i<len; i++)
					dprintf("%2x",buf2[i]);
			} else {
				dprintf("\r\n");
				break;
			}
		}
	}
}

void uart_test2(void)
{
	int ret;
	char buf[1024];
	int len;
	int total_len = 0;
	volatile int j;

	/* uart0 tx <---> uart 3 rx 
	 * to test baudrate is right
	 */
	dprintf("capture from uart0 start");

	for(j =0 ; j<2000; j++)
		;

	uart_close(3);
	ret = uart_open(3,115200,8,'n',1);

	/* send to uart 3 */
	dprintf("abcdefghijklmnopq1234325454545\r\n");

	for(j =0 ; j<2000; j++)
		;

	memset(buf,0,sizeof(buf));
	while((len = uart_rx(3, buf + total_len, 1024 - total_len))) {
		total_len += len;
	}

	uart_close(3);

	dprintf("%s",buf);
	dprintf("capture stop");
	for(;;)
		;
}

void uart_test3(void)
{
	int ret;
	char buf[1024];
	int i;
	volatile int j;

	for(i = 0; i<sizeof(buf); i++)
		buf[i] = i & 0xff;

	/* uart3 tx test */
	ret = uart_open(3,115200,8,'n',1);
	dprintf("open uart 3 ret:%d\r\n", ret);

	for(;;) {
		uart_tx_str(3, "AT\r");
		while(!uart_tx_finish(3))
			;

		while(!(ret = uart_rx(3, buf, sizeof(buf))))
			;

		if(ret < 0) {
			RERR();
			continue;
		}

		j = 1000;
		while(j--)
			;
		ret += uart_rx(3, buf + ret, sizeof(buf));
		if(ret > 0) {
			buf[ret] = 0;
			dprintf("ret:%d buf:%s\r\n",ret, buf);
		} else {
			dprintf("uart rx 3 ret:%d\r\n",ret);
		}
	}

	uart_close(3);
}
