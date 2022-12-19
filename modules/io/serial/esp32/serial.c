/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	to do:
		!! format: ascii
		onError - framing errors?
		drain transmit? (onWritable when threshold is 0)
		flush transmit, flush receive?
		other pins, hardware flow control
*/


#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/uart.h"
#include "hal/uart_hal.h"
#include "hal/uart_types.h"
//#include "soc/uart_caps.h"
#include "soc/uart_struct.h"

// local versions of UART register management to avoid issues with uart.c
#define uart_disable_intr_mask(dev, disable_mask) _uart_disable_intr_mask(dev, disable_mask)
#define uart_enable_intr_mask(dev, enable_mask) _uart_enable_intr_mask(dev, enable_mask)
#define uart_enable_rx_intr(dev) _uart_enable_rx_intr(dev)
#define uart_disable_rx_intr(dev) _uart_disable_rx_intr(dev)
#define uart_disable_tx_intr(dev) _uart_disable_tx_intr(dev)
#define uart_enable_tx_intr(dev, enable, thresh) _uart_enable_tx_intr(dev, enable, thresh)

static void _uart_disable_intr_mask(uart_dev_t *dev, uint32_t disable_mask);
static void _uart_enable_intr_mask(uart_dev_t *dev, uint32_t enable_mask);
static void _uart_enable_rx_intr(uart_dev_t *dev);
static void _uart_disable_rx_intr(uart_dev_t *dev);
static void _uart_disable_tx_intr(uart_dev_t *dev);
static void _uart_enable_tx_intr(uart_dev_t *dev, int enable, int thresh);

typedef struct SerialRecord SerialRecord;
typedef struct SerialRecord *Serial;

struct SerialRecord {
	xsSlot		obj;
	uint8_t		isReadable;
	uint8_t		isWritable;
	uint8_t		format;
	uint8_t		uart;
	uint8_t		useCount;
	uint8_t		txInterruptEnabled;
	uint8_t		hasOnReadableOrWritable;
	uart_dev_t	*uart_reg;
	uint32_t	transmit;
	uint32_t	receive;
	xsMachine	*the;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
};

static void ICACHE_RAM_ATTR serial_isr(void * arg);
static void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsSerialHooks = {
	xs_serial_destructor,
	xs_serial_mark,
	NULL
};

#define kTransmitTreshold (10)		// could be build-time or runtime configurable

void xs_serial_constructor(xsMachine *the)
{
	Serial serial;
	int baud;
	uint8_t hasReadable, hasWritable, format;
	xsSlot *onReadable, *onWritable;
	esp_err_t err;
	uart_config_t uartConfig = {0};
	int uart = 0;
	uint32_t transmit = UART_PIN_NO_CHANGE, receive = UART_PIN_NO_CHANGE;

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_transmit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_transmit);
		transmit = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(transmit))
			xsUnknownError("in use");
	}

	if (xsmcHas(xsArg(0), xsID_receive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_receive);
		receive = builtinGetPin(the, &xsVar(0));
		if (!builtinIsPinFree(receive))
			xsUnknownError("in use");
	}
	else if (UART_PIN_NO_CHANGE == transmit)
		xsUnknownError("invalid");

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		uart = builtinGetSignedInteger(the, &xsVar(0));
		if ((uart < 0) || (uart >= UART_NUM_MAX))
			xsRangeError("invalid port");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_baud);
	baud = xsmcToInteger(xsVar(0));
	if ((baud <= 0) || (baud > 20000000))
		xsRangeError("invalid baud");

	onReadable = builtinGetCallback(the, xsID_onReadable);
	onWritable = builtinGetCallback(the, xsID_onWritable);

	hasReadable = (UART_PIN_NO_CHANGE != receive) && onReadable;
	hasWritable = (UART_PIN_NO_CHANGE != transmit) && onWritable;

	builtinInitializeTarget(the);

	format = builtinInitializeFormat(the, kIOFormatNumber);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("invalid format");

	if (uart_is_driver_installed(uart))
		xsRangeError("port in use");

	uartConfig.baud_rate = baud;
	uartConfig.data_bits = UART_DATA_8_BITS;
	uartConfig.parity = UART_PARITY_DISABLE;
	uartConfig.stop_bits = UART_STOP_BITS_1;
	uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uartConfig.rx_flow_ctrl_thresh = 120;		// unused. no hardware flow control.
//	uartConfig.use_ref_tick = 0;	 // deprecated in 4.x

	err = uart_param_config(uart, &uartConfig);
	if (err)
		xsUnknownError("uart failed");

	err = uart_set_pin(uart, transmit, receive, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err)
		xsUnknownError("uart failed");

	serial = c_malloc((hasReadable || hasWritable) ? sizeof(SerialRecord) : offsetof(SerialRecord, the));
	if (!serial)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, serial);

	serial->obj = xsThis;

	serial->transmit = transmit;
	serial->receive = receive;
	serial->format = format;
	serial->uart = (uint8_t)uart;
	serial->isReadable = 0;
	serial->isWritable = 0;
#if UART_NUM_MAX > 2
	if (2 == uart)
		serial->uart_reg = &UART2;
	else
#endif
		serial->uart_reg = uart ? &UART1 : &UART0;
	serial->useCount = 1;
	serial->hasOnReadableOrWritable = hasReadable || hasWritable;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSerialHooks);

	if (serial->hasOnReadableOrWritable) {
		serial->the = the;

		// store callbacks & configure interrupts
		err = esp_intr_alloc(uart_periph_signal[uart].irq, 0, serial_isr, serial, NULL);
		if (err)
			xsUnknownError("uart_isr_register failed");

		if (hasReadable) {
			serial->onReadable = onReadable;

			uart_enable_rx_intr(serial->uart_reg);
			uart_set_rx_timeout(serial->uart, 4);
		}
		else {
			uart_disable_rx_intr(serial->uart_reg);
			serial->onReadable = NULL;
		}

		if (hasWritable) {
			serial->onWritable = onWritable;

			uart_enable_tx_intr(serial->uart_reg, 1, kTransmitTreshold);
		}
		else {
			uart_disable_tx_intr(serial->uart_reg);
			serial->onWritable = NULL;
		}
	}

	xsRemember(serial->obj);

	if (UART_PIN_NO_CHANGE != transmit)
		builtinUsePin(transmit);
	if (UART_PIN_NO_CHANGE != receive)
		builtinUsePin(receive);
}

void xs_serial_destructor(void *data)
{
	Serial serial = data;
	if (!serial) return;

	uart_disable_tx_intr(serial->uart_reg);
	uart_disable_rx_intr(serial->uart_reg);

	uart_isr_free(serial->uart);

	if (UART_PIN_NO_CHANGE != serial->transmit)
		builtinFreePin(serial->transmit);
	if (UART_PIN_NO_CHANGE != serial->receive)
		builtinFreePin(serial->receive);

	if (0 == __atomic_sub_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST))
		c_free(serial);
}

void xs_serial_close(xsMachine *the)
{
	Serial serial = xsmcGetHostData(xsThis);
	if (serial && xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks)) {
		xsForget(serial->obj);
		xs_serial_destructor(serial);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_serial_get_format(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	builtinGetFormat(the, serial->format);
}

void xs_serial_set_format(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
		xsRangeError("unimplemented");
	serial->format = format;
}

void xs_serial_read(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int available;

	available = uart_ll_get_rxfifo_len(serial->uart_reg);
	if (0 == available)
		return;

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		uart_ll_read_rxfifo(serial->uart_reg, &byte, 1);

		xsmcSetInteger(xsResult, byte);
	}
	else {
		uint8_t *buffer;
		int requested;
		xsUnsignedValue byteLength;
		uint8_t allocate = 1;
		
		if (0 == xsmcArgc)
			requested = available;
		else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
			xsResult = xsArg(0);
			xsmcGetBufferWritable(xsResult, (void **)&buffer, &byteLength);
			requested = (int)byteLength;
			if (requested > available)
				requested = available;
			allocate = 0;
			xsmcSetInteger(xsResult, requested);
		}
		else
			requested = xsmcToInteger(xsArg(0));

		if (requested <= 0) 
			xsUnknownError("invalid");

		if (allocate)
			buffer = xsmcSetArrayBuffer(xsResult, NULL, requested);

		uart_ll_read_rxfifo(serial->uart_reg, buffer, requested);
	}

	if (serial->onReadable)
		uart_enable_rx_intr(serial->uart_reg);
}

void xs_serial_write(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);
	int count = uart_ll_get_txfifo_len(serial->uart_reg);

	if (kIOFormatNumber == serial->format) {
		uint8_t byte;

		if (0 == count)
			xsUnknownError("output full");

		byte = (uint8_t)xsmcToInteger(xsArg(0));
		uart_ll_write_txfifo(serial->uart_reg, &byte, 1);
	}
	else {
		void *buffer;
		xsUnsignedValue requested;

		xsmcGetBufferReadable(xsArg(0), &buffer, &requested);
		if (requested > count)
			xsUnknownError("output full");

		uart_ll_write_txfifo(serial->uart_reg, buffer, requested);
	}

	if (serial->onWritable && !serial->txInterruptEnabled) {
		serial->txInterruptEnabled = 1;
		uart_enable_tx_intr(serial->uart_reg, 1, kTransmitTreshold);
	}
}

void xs_serial_purge(xsMachine *the)
{
	Serial serial = xsmcGetHostDataValidate(xsThis, (void *)&xsSerialHooks);

	uart_ll_txfifo_rst(serial->uart_reg);
	uart_ll_rxfifo_rst(serial->uart_reg);
}

void ICACHE_RAM_ATTR serial_isr(void * arg)
{
	Serial serial = arg;
	uint8_t post = 0;
	int status = serial->uart_reg->int_st.val;

	if ((status & UART_INTR_TXFIFO_EMPTY) && serial->onWritable) {
		post |= (0 == serial->isWritable);
		serial->isWritable = 1;
		serial->txInterruptEnabled = 0;
		uart_disable_tx_intr(serial->uart_reg);
	}

	if ((status & (UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL)) && serial->onReadable) {
		post |= (0 == serial->isReadable);
		serial->isReadable = 1;
		uart_disable_rx_intr(serial->uart_reg);
	}

	if (post) {
		__atomic_add_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST);
		modMessagePostToMachineFromISR(serial->the, serialDeliver, serial);
	}
}

void serialDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	Serial serial = (Serial)refcon;
	int count;

	if (0 == __atomic_sub_fetch(&serial->useCount, 1, __ATOMIC_SEQ_CST)) {
		c_free(serial);
		return;
	}

	if (serial->isReadable) {
		serial->isReadable = 0;
		count = uart_ll_get_rxfifo_len(serial->uart_reg);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onReadable), serial->obj, xsResult);
			xsEndHost(the);
		}
		uart_enable_rx_intr(serial->uart_reg);
	}

	if (serial->isWritable) {
		serial->isWritable = 0;
		count = uart_ll_get_txfifo_len(serial->uart_reg);
		if (count) {
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(serial->onWritable), serial->obj, xsResult);
			xsEndHost(the);
		}
	}
}

void xs_serial_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Serial serial = it;

	if (!serial->hasOnReadableOrWritable)
		return;

	if (serial->onReadable)
		(*markRoot)(the, serial->onReadable);
	if (serial->onWritable)
		(*markRoot)(the, serial->onWritable);
}

/*
	functions below mirror functons of same names in ESP-IDF uart.c
	
	ESP-IDF v4.4 introduced p_uart_obj which made it impossible to use the functions in uart.c
		without also using the uart driver
	
	https://github.com/Moddable-OpenSource/moddable/issues/931
*/

static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

void _uart_disable_intr_mask(uart_dev_t *dev, uint32_t disable_mask)
{
    portENTER_CRITICAL(&spinlock);
    uart_ll_disable_intr_mask(dev, disable_mask);
    portEXIT_CRITICAL(&spinlock);
}

void _uart_enable_intr_mask(uart_dev_t *dev, uint32_t enable_mask)
{
    portENTER_CRITICAL(&spinlock);
    uart_ll_clr_intsts_mask(dev, enable_mask);
    uart_ll_ena_intr_mask(dev, enable_mask);
    portEXIT_CRITICAL(&spinlock);
}

void _uart_enable_rx_intr(uart_dev_t *dev)
{
    _uart_enable_intr_mask(dev, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
}

void _uart_disable_rx_intr(uart_dev_t *dev)
{
    _uart_disable_intr_mask(dev, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
}

void _uart_disable_tx_intr(uart_dev_t *dev)
{
    _uart_disable_intr_mask(dev, UART_INTR_TXFIFO_EMPTY);
}

void _uart_enable_tx_intr(uart_dev_t *dev, int enable, int thresh)
{
    if (enable == 0) {
		portENTER_CRITICAL(&spinlock);
        uart_ll_disable_intr_mask(dev, UART_INTR_TXFIFO_EMPTY);
		portEXIT_CRITICAL(&spinlock);
    } else {
        uart_ll_clr_intsts_mask(dev, UART_INTR_TXFIFO_EMPTY);
		portENTER_CRITICAL(&spinlock);
        uart_ll_set_txfifo_empty_thr(dev, thresh);
        uart_ll_ena_intr_mask(dev, UART_INTR_TXFIFO_EMPTY);
		portEXIT_CRITICAL(&spinlock);
    }
}
