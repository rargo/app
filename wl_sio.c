#include <base.h>
#include <ucos.h>
#include "ppp.h"
#include <lwip/sio.h>
#include <arch/sys_arch.h>
#include <uart.h>

/* XXX currently for simple:sio[n] maps uart channel n*/
#define MAX_SIO_CHANNEL 4

typedef struct sio_s {
	int channel; /* uart channel number */
} sio_t;

sio_t sio[MAX_SIO_CHANNEL];

/* for ppp serial support */

#if 0
/* if ppp link status change, function will be call to handle 
 * serial port
 */
void pppLinkStatusCallBack(void *ctx, int errCode, void *arg)
{
	RDIAG(SIO_DEBUG,"errCode:%d",errCode);
	
	switch(errCode) 
	{


	}
}
#endif

/**
 * Opens a serial device for communication.
 * 
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t wl_sio_open(u8_t devnum)
{
	if(uart_open(devnum, 115200, 8, 'n', 1))
		return NULL;

	sio[devnum].channel = devnum;

	return &sio[devnum];
}

#if 0
/**
 * Sends a single character to the serial device.
 * 
 * @param c character to send
 * @param fd serial device handle
 * 
 * @note This function will block until the character can be sent.
 */
void wl_sio_send(u8_t c, sio_fd_t fd)
{
	sio_t *s = (sio_t *)fd;
	int ret;

	if(fd == NULL) {
		RERR();
		return;
	}

	RDIAG(SIO_DEBUG);
	ret = uart_tx_sleep(s->channel,(char *)&c,1);
	if(ret < 0) {
		RERR("ret:%d",ret);
	}
}

/**
 * Receives a single character from the serial device.
 * 
 * @param fd serial device handle
 * 
 * @note This function will block until a character is received.
 */
u8_t wl_sio_recv(sio_fd_t fd)
{
	sio_t *s = (sio_t *)fd;
	char c = '\0';
	int recv_len;
	
	RDIAG(SIO_DEBUG);

	if(fd == NULL) {
		RERR();
		return 0;
	}
	
	recv_len = uart_rx_sleep(s->channel, &c, 1);
	if(recv_len < 0) {
		RERR("recv_len:%d",recv_len);
	} 

	return c;
}
#endif

/**
 * Reads from the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 * 
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
//TODO in this function
//we need to filter out the module events(such as RING, ^SYSSHUTDOWN)
//from the ppp packet
u32_t wl_sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
	int recv_len;
	sio_t *s = (sio_t *)fd;
	
	RDIAG(SIO_DEBUG);

	if(fd == NULL) {
		RERR();
		return 0;
	}
	
	recv_len = uart_rx_sleep(s->channel, (char *)data, len);
	if(recv_len < 0) {
		RERR("recv_len:%d",recv_len);
		return 0;
	} else
		return recv_len;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t wl_sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
	sio_t *s = (sio_t *)fd;
	
	RDIAG(SIO_DEBUG);

	return uart_rx(s->channel, (char *)data, len);
}

/**
 * Writes to the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 * 
 * @note This function will block until all data can be sentsleep_abort(s->channel);
}
