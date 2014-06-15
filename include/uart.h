#ifndef UART_H
#define UART_H

int serial_init(void);
char serial_getc(void);
void serial_putc(const char c);
void serial_puts(const char *s);
void serial_send(const char *s, int len);
void dprintf(char *fmt, ...);
void uart_init(void);
int uart_open(int ch, unsigned int baudrate,unsigned int databits, \
		char parity,unsigned int stopbits);
int uart_tx(int ch, const char *buf, int len);
int uart_tx_str(int ch, const char *str);
int uart_printf(int ch, const char *fmt, ...);
int uart_rx(int ch, char *buf, int len);
int uart_tx_finish(int ch);
int uart_tx_buffer_empty(int ch);
int uart_tx_buffer_full(int ch);
int uart_tx_buffer_clear(int ch);
int uart_rx_buffer_clear(int ch);
void uart_close(int ch);

int uart_tx_sleep(int ch, const char *buf, int len);
int uart_rx_sleep(int ch, char *buf, int len);
void uart_tx_sleep_abort(int ch);
void uart_rx_sleep_abort(int ch);
void uart_sleep_abort(int ch);

void uart_test(void);
void uart_test2(void);
void uart_test3(void);

#endif
