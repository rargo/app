#ifndef DLOG_H
#define DLOG_H
void dlog(char *fmt, ...);
void dlog_hex(const void* buffer, int size);
int dlog_print(void);
#endif	
