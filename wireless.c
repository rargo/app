#include <base.h>
#include <uart.h>

#define WL_CHANNEL 3

#define WL_NOT_CONNECT  -1
#define WL_NOT_CMD_MODE -2

int wl_command(char *in, char *out, int timeout)
{
	char buf[256];
	int ret;
	int len;
	int j;

	RDIAG(WL_DEBUG,"WL COMMAND:%s", in);

	uart_tx_str(WL_CHANNEL, in);
	while(!uart_tx_finish(WL_CHANNEL))
		;

	/* wait command response */
	len = 0;
	while(1) {
		/* XXX add timeout here */
		ret = uart_rx(WL_CHANNEL, buf + len, sizeof(buf) - len);
		if(ret > 0) {
			len += ret;
			break;
		} else if(ret < 0)
			RERR();
		else
			continue;
	}

	j = 0;
	while(1) {
		ret = uart_rx(WL_CHANNEL, buf + len, sizeof(buf) - len);
		if(ret > 0) {
			len += ret;
			j = 0;
		} else if (ret == 0) {
			if(j++ > 1000)
				break;
		} else
			RERR();
	}
	buf[len] = '\0';

	RDIAG(WL_DEBUG,"WL RESPONSE[%d]:%s", len, buf);
	/* check response */
	if(strstr(buf, out) != NULL) {
		RDIAG(WL_DEBUG,"success"); 
		return 0;
	} else {
		//RERR("fail"); 
		RASSERT(0);
		return -2;// XXX fix return value;
	}
}

/* connect gprs */
int wl_connect(void)
{
	int ret;
	ret = uart_open(WL_CHANNEL, 115200, 8, 'n', 1);
	RDIAG(WL_DEBUG,"uart open %d ret:%d",WL_CHANNEL, ret);

	wl_command("ATE0\r", "OK", 0);
	wl_command("ATS0=0\r", "OK", 0);
	wl_command("AT+CSQ\r", "OK", 0);
	wl_command("AT+CGREG?\r", "+CGREG:", 0); //register gprs network
	wl_command("AT+CGATT?\r", "CGATT:", 0); //attach this me to gprs
	wl_command("AT+CGDCONT=1,\"IP\",\"CMNET\"\r", "OK",0); //define PDP context cid=1
	wl_command("ATD*99***1#\r", "CONNECT", 0); //try connect to gprs use cid 1
	uart_close(WL_CHANNEL);
	return 0;
}

/* when disconnect, how can we shutdown all sockets??? */
int wl_disconnect(void)
{

}

#define UNKNOWN_MODE -1
#define CMD_MODE 0
#define DATA_MODE 1
static int wl_mode = UNKNOWN_MODE;
static int wl_mode_get(void)
{

}

static int wl_mode_switch(int mode)
{


}

//return 0 if no more message
//must in cmd_mode
int wl_read_message(unsigned char *buf, int len)
{

	return 0; 
}

int wl_write_message(char *number, char *buf, int len)
{

}

// read signal level
int  wl_read_signal(void)
{

}

// enable or disable auto answer
int wl_auto_answer(int auto_answer)
{

}

// answer phone call
int wl_answer(void)
{

}

// dial number
int wl_dial(char *number, int timeout)
{

}

// handoff call
int wl_handoff(void)
{

}

// set call volume
int wl_set_vol(int vol)
{

}

// wireless thread, control wireless status change
// process wl command
// will disconnect data link every 5 seconds
// check if there's incoming message
// if incomming call interrupt data link
// reconnect it
void wl_thread(void *arg)
{

}

int wl_init(void)
{
	/* create wireless thread */
	return 0;
}

