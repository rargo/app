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
sio_fd_t sio_open(u8_t devnum)
{
	return wl_sio_open(devnum);
}

/**
 * Sends a single character to the serial device.
 * 
 * @param c character to send
 * @param fd serial device handle
 * 
 * @note This function will block until the character can be sent.
 */
//only slipif.c need this function
void sio_send(u8_t c, sio_fd_t fd)
{
	//return wl_sio_send(c, fd);
}

/**
 * Receives a single character from the serial device.
 * 
 * @param fd serial device handle
 * 
 * @note This function will block until a character is received.
 */
//no caller found?!!
u8_t sio_recv(sio_fd_t fd)
{
	//return wl_sio_recv(fd);
	return -1;
}

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
//slipif and pppos need it
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
	return wl_sio_read(fd, data, len);
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
//only slipif.c need this function
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
	return wl_sio_tryread(fd, data, len);
}

/**
 * Writes to the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 * 
 * @note This function will block until all data can be sent.
 */
//pppos need this function
u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len)
{
	return wl_sio_write(fd, data, len);
}

/**
 * Aborts a blocking sio_read() call.
 * 
 * @param fd serial device handle
 */
//pppos need this function
void sio_read_abort(sio_fd_t fd)
{
	return wl_sio_read_abort(fd);
}
