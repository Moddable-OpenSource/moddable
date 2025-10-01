
#include "xs.h"
#include "xsHost.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "mc.defines.h"

#ifndef DEBUGGER_SPEED
	#define DEBUGGER_SPEED 115200		// in .dts?
#endif

#define DEBUGGER_STACK 2048
#define DEBUGGER_PRIORITY	3

k_tid_t gDebugTID;
struct k_thread gDebugThread;
K_THREAD_STACK_DEFINE(gDebugStack, DEBUGGER_STACK);

#define DEBUG_QUEUE_LEN			2

#define RING_BUF_SIZE	1024
static struct ring_buf m_rx_ringbuf;
static uint8_t rx_ringbuf_buffer[RING_BUF_SIZE];

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static uint8_t gNotifyOutstanding = 0;
struct k_msgq dbgServiceQueue;

static void die(char *x) {
	while (1)
		;
}

void serial_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(uart_dev))
		return;

	while (uart_irq_rx_ready(uart_dev)) {
		uint8_t buf[32];
		
		uint32_t space = ring_buf_space_get(&m_rx_ringbuf);
		if (space == 0)
			die("rx ring buffer full");

		uint32_t use = (space < sizeof(buf)) ? space : sizeof(buf);
		uint32_t read = uart_fifo_read(uart_dev, buf, use);
		if (read > 0) {
			uint32_t written = ring_buf_put(&m_rx_ringbuf, buf, read);
			if (written != read)
				die("rx ring buffer full");
			if (!gNotifyOutstanding) {
				gNotifyOutstanding = 1;
				k_msgq_put(&dbgServiceQueue, &gNotifyOutstanding, K_NO_WAIT);
			}
		}
	}
}

static void debugLoop(void *a, void *b, void *c)
{
	uint32_t msg;
	uint32_t *running = a;
	
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	if (ret < 0) {
		if ((ret == -ENOTSUP) || (ret == -ENOSYS))
			printk("Interrupt-driven UART not enabled.\n");
		else
			printk("error setting UART callback: %d\n", ret);
		*running = -1;
		return;
	}

	uart_irq_rx_enable(uart_dev);
	*running = 1;

	while (true) {
		int err = k_msgq_get(&dbgServiceQueue, &msg, K_MSEC(10000));
		if (err) continue;

		gNotifyOutstanding = 0;
#ifdef mxDebug
		if (ring_buf_size_get(&m_rx_ringbuf) > 0)
			fxReceiveLoop();
#endif
	}
}


void setupDebugger(uint32_t *running)
{
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!\n");
		*running = -1;
		return;
	}

	k_msgq_alloc_init(&dbgServiceQueue, sizeof(uint8_t), DEBUG_QUEUE_LEN);

	// initial ringbuf
	ring_buf_init(&m_rx_ringbuf, sizeof(rx_ringbuf_buffer), rx_ringbuf_buffer);

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
	uint32_t len;

	len = ring_buf_get(&m_rx_ringbuf, &c, 1);
	if (1 == len)
		return c;

	return -1;
}


