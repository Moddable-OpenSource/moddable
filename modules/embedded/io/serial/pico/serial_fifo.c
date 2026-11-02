#include "xsPlatform.h"
#include <stdint.h>
#include "serial_fifo.h"

#define __INLINE inline

static __INLINE uint32_t F_length(fifo_t *fifo) {
	uint32_t tmp = fifo->read;
	return fifo->write - tmp;
}

static __INLINE void F_put(fifo_t *fifo, uint8_t c) {
	fifo->buf[fifo->write & fifo->size_mask] = c;
	fifo->write++;
}

static __INLINE void F_get(fifo_t *fifo, uint8_t *c) {
	*c = fifo->buf[fifo->read & fifo->size_mask];
	fifo->read++;
}

void fifo_flush(fifo_t *fifo) {
	fifo->read = fifo->write;
}

uint32_t fifo_length(fifo_t *fifo) {
	return F_length(fifo);
}

uint32_t fifo_full(fifo_t *fifo) {
	if (0 == (fifo->size_mask - F_length(fifo) + 1))
		return 1;
	return 0;
}

int fifo_get(fifo_t *fifo, uint8_t *c) {
	if (F_length(fifo) == 0)
		return -1;
	F_get(fifo, c);
	return 0;
}

int fifo_put(fifo_t *fifo, uint8_t c) {
	if (0 == (fifo->size_mask - F_length(fifo) + 1))
		return -1;
	F_put(fifo, c);
	return 0;
}


int fifo_init(fifo_t *fifo, uint8_t *buf, uint32_t size) {
	if (0 == buf)
		return -1;

	if (! ((0 != size) && (0 == ((size - 1) & size))))
		return -2;		// bad size - needs to be base 2

	fifo->buf = buf;
	fifo->size_mask = size - 1;
	fifo->read = 0;
	fifo->write = 0;

	return 0;
}

