#include <base.h>
#include <uart.h>

#define WL_CHANNEL 3

int wl_command(char *in, char *out, int timeout)
{
	char buf[256];
	int ret;
	int len;
	int j;

	RDIAG(LEGACY_DEBUG,"wl command:%s", in);

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

	RDIAG(LEGACY_DEBUG,"wl response[%d]:%s", len, buf);
	/* check response */
	if(strstr(buf, out) != NULL) {
		RDIAG(LEGACY_DEBUG,"success"); 
		return 0;
	} else {
		//RERR("fail"); 
		RASSERT(0);
		return -2;// XXX fix return value;
	}
}

int wl_init(void)
{
	return 0;
}

int wl_connect(void)
{
	int ret;
	ret = uart_open(WL_CHANNEL, 115200, 8, 'n', 1);
	RDIAG(LEGACY_DEBUG,"uart open %d ret:%d",WL_CHANNEL, ret);

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
