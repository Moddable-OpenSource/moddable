
#ifndef __SERIAL_FIFO__
#define __SERIAL_FIFO__

typedef struct {
	uint8_t		*buf;
	uint32_t	size_mask;
	volatile uint32_t read;
	volatile uint32_t write;
} fifo_t;

uint32_t fifo_length(fifo_t *fifo);
uint32_t fifo_full(fifo_t *fifo);
int fifo_put(fifo_t *fifo, uint8_t c);
int fifo_get(fifo_t *fifo, uint8_t *c);
void fifo_flush(fifo_t *fifo);
int fifo_init(fifo_t *fifo, uint8_t *buf, uint32_t size);

#endif // __SERIAL_FIFO__
