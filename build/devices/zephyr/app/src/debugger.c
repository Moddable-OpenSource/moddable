
#include "xs.h"
#include "xsHost.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "mc.defines.h"

#include "serial_fifo.h"

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 115200		// in .dts?
#endif

#define DEBUGGER_STACK 2048
#define DEBUGGER_PRIORITY	3

k_tid_t gDebugTID;
struct k_thread gDebugThread;
K_THREAD_STACK_DEFINE(gDebugStack, DEBUGGER_STACK);

#define DEBUG_QUEUE_LEN			2

#define FIFO_SIZE	1024
static fifo_t	m_rx_fifo;
static uint8_t *rx_fifo_buffer;

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static int gNotifyOutstanding = 0;
struct k_msgq dbgServiceQueue;
#define  UART_RCV	0b0001
#define  UART_TX	0b0010

void die(char *x) {
	uint8_t DIE_BUF[256];
	c_strncpy(DIE_BUF, x, 256);
	while (1) {
	}
}
#define DIE(x, ...) die(x)

uint32_t fillBufFromFifo(fifo_t *fifo, uint8_t *buf, uint32_t bufSize) {
	int i=0;
	while (i<bufSize && (0 != fifo_get(fifo, buf + i)))
		i++;
	return i;
}

void serial_cb(const struct device *dev, void *user_data)
{
	uint32_t msg = 0;
	int amt;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	while (uart_irq_rx_ready(uart_dev)) {
		uint8_t c;
		msg |= UART_RCV;
		while (1 == (amt = uart_fifo_read(uart_dev, &c, 1))) {
			if (0 != fifo_put(&m_rx_fifo, c))
				die("rx fifo full");
		}
	}

	if (msg && !gNotifyOutstanding) {
		gNotifyOutstanding = 1;
		k_msgq_put(&dbgServiceQueue, &msg, K_NO_WAIT);
	}
}

static void debugLoop(void *a, void *b, void *c)
{
	int err;
	uint32_t msg;
	uint32_t *running = a;

	uart_irq_rx_enable(uart_dev);
	*running = 1;

	while (true) {
		if (gNotifyOutstanding)
			err = 0;
		else
			err = k_msgq_get(&dbgServiceQueue, &msg, K_MSEC(10000));
		if (0 == err) {
			gNotifyOutstanding = 0;
#ifdef mxDebug
			if (fifo_length(&m_rx_fifo))
				fxReceiveLoop();
#endif
		}
	}
}


void setupDebugger(uint32_t *running)
{
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!\n");
		return;
	}

	k_msgq_alloc_init(&dbgServiceQueue, sizeof(uint32_t), DEBUG_QUEUE_LEN);

	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART not enabled.\n");
		}
		else if (ret == -ENOSYS) {
			printk("Interrupt-driven UART not supported.\n");
		}
		else {
			printk("error setting UART callback: %d\n", ret);
		}
		return;
	}

	// make fifos
	rx_fifo_buffer = c_malloc(FIFO_SIZE);
	fifo_init(&m_rx_fifo, rx_fifo_buffer, FIFO_SIZE);

	// make debugger thread
	gDebugTID = k_thread_create(&gDebugThread, gDebugStack, K_THREAD_STACK_SIZEOF(gDebugStack), debugLoop, running, NULL, NULL, DEBUGGER_PRIORITY, 0, K_NO_WAIT);
}

void ESP_put(uint8_t *c, int count)
{
	while (count) {
		ESP_putc(*c++);
		count--;
	}
}

void ESP_putc(int c)
{
	uint8_t ch = c;

	uart_poll_out(uart_dev, ch);
}

int ESP_getc(void)
{
	uint8_t c;
	int err;

	err = fifo_get(&m_rx_fifo, &c);
	if (0 == err)
		return c;

	return -1;
}


