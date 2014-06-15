#ifndef CYCLE_BUFFER_H
#define CYCLE_BUFFER_H
typedef struct buf {
	char *name;
	unsigned char *buf;
	int size;
	int head;
	int tail;
    int scan_pos;
} buf_t;

#define DECLARE_CYCLE_BUFFER(name, buf, size) buf_t name = {#name, buf, (size), 0, 0, 0}

void push_buf(buf_t *b, unsigned char *buf, int len);
int pop_buf(buf_t *b, unsigned char *buf, int len);
#endif
