#include <cycle_buffer.h>
#include <stdarg.h>
#include <uart.h>
#include <string.h>

/* place LOG_PRINT here not in base.h, 
 * to avoid affect many file recompile 
 */
//#define LOG_PRINT  LOG_REALTIME
#define LOG_PRINT  LOG_CYCLE_BUF

/* print log real time, uart poll */
#define LOG_REALTIME 0
/* print log to a buf, idle task output it */
#define LOG_CYCLE_BUF 1

#define LOG_CYCLE_BUFFER_SIZE 64*1024
static unsigned char log_buffer[LOG_CYCLE_BUFFER_SIZE];

DECLARE_CYCLE_BUFFER(log_buf, log_buffer, LOG_CYCLE_BUFFER_SIZE);

int vsprintf(char *buf, const char *fmt, va_list args);
static char sbuffer[1024];
void dlog(char *fmt, ...)
{
	va_list varg;

	//memset(sbuffer, 0, sizeof(sbuffer));
	va_start(varg, fmt);
	vsprintf(sbuffer, fmt, varg);
	va_end(varg);

#if LOG_PRINT == LOG_CYCLE_BUF
	push_buf(&log_buf, sbuffer, strlen(sbuffer));
#elif LOG_PRINT == LOG_REALTIME
	serial_puts(sbuffer);
#else
#error "log metthod error!"
#endif
}

int dlog_print(void)
{
	unsigned char buf[2048];
	int len;
#if LOG_PRINT == LOG_CYCLE_BUF
	if((len = pop_buf(&log_buf, buf, sizeof(buf))) > 0) {
		serial_send(buf, len);
		return len;
	}
#endif

	return 0;
}

void dlog_hex(const void* buffer, int size)
{
		const int line_len = 16;

		dlog("=============================================================================\r\n");
		int i;
		for(i=0; i<size; i++) {
			if(i%line_len==0) {
				dlog("0x%04X  ",i);
			}
			dlog("%02X ",(unsigned char)((unsigned char*)buffer)[i]);
			if((i+1)%(line_len/2)==0) {
				dlog("  ");
			}
			if((i+1)%line_len==0) {
				dlog(" ");
				int j;
				for(j=i-line_len+1; j<=i; j++) {
					unsigned char c = ((unsigned char*)buffer)[j];
					if(c>=32 && c<=124)
						dlog("%c",c);
					else
						dlog(".");
				}
				dlog("\r\n");
			}
		}
		if(i%line_len!=0) {
			int j;
			for(j=i; j<((i+line_len-1)/line_len)*line_len; j++) {
				dlog("   ");
				if((j+1)%(line_len/2)==0) {
					dlog("  ");
				}
			}
			dlog(" ");
			for(j=(i/line_len)*line_len; j<i; j++) {
				unsigned char c = ((unsigned char*)buffer)[j];
				if(c>=32 && c<=124)
					dlog("%c",c);
				else
					dlog(".");
			}
			dlog("\r\n");
		}
		dlog("=============================================================================\r\n");
}
