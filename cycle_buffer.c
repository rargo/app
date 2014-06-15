#include <cycle_buffer.h>
#include <string.h>
#include <uart.h>


#define COMSTR(fmt, ...)
//#define COMSTR dprintf

static int buf_len(buf_t *b, int start_pos, int end_pos)
{
	int mount;
	if(end_pos >= start_pos)
		mount = end_pos - start_pos;
	else
		mount = b->size - start_pos + end_pos;

	return mount;
}

void push_buf(buf_t *b, unsigned char *buf, int len)
{
	int bfree;
	unsigned int sr;

	if(!len)
		return;

	bfree = b->size - buf_len(b, b->head, b->tail);
	COMSTR("%s ===> before push head:%d,scan_pos:%d, tail:%d",b->name, b->head, b->scan_pos, b->tail);

	if(len > bfree) {
		COMSTR("error buf overflow!");
		return;
	}

	COMSTR("push %d bytes", len);

	if((b->tail < b->head) || (b->size - b->tail >= len)) {
		memcpy(b->buf + b->tail, buf, len);
		//COMHEX(buf,len);
	} else {
		int taillen;
		taillen = b->size - b->tail;
		memcpy(b->buf + b->tail, buf, taillen);
		//COMHEX(buf,taillen);
		memcpy(b->buf, buf + taillen, len - taillen);
		//COMHEX(buf + taillen, len - taillen);
	}

	b->tail = (b->tail + len) % b->size;
	COMSTR("%s <=== after push head:%d,scan_pos:%d, tail:%d", b->name, b->head, b->scan_pos, b->tail);
}

int pop_buf(buf_t *b, unsigned char *buf, int len)
{
	int bmount;

	bmount = buf_len(b, b->head, b->tail);
	if(bmount == 0)
		return 0;

	if(bmount < len)
		len = bmount; /* only copy available data */
	COMSTR("%s ===> before pop head:%d,scan_pos:%d, tail:%d",b->name, b->head, b->scan_pos, b->tail);
	COMSTR("pop %d bytes", len);

	if((b->head < b->tail) || (b->size - b->head >= len)) {
		memcpy(buf, b->buf + b->head, len);
		//COMHEX(b->buf + b->head, len);
	} else {
		int taillen;
		taillen = b->size - b->head;
		memcpy(buf, b->buf + b->head, taillen);
		//COMHEX(b->buf + b->head, taillen);
		memcpy(buf + taillen, b->buf, len - taillen);
		//COMHEX(b->buf, len - taillen);
	}

	b->head = (b->head + len) % b->size;
	COMSTR("%s <=== after pop head:%d,scan_pos:%d, tail:%d", b->name, b->head, b->scan_pos, b->tail);

	return len;
}
