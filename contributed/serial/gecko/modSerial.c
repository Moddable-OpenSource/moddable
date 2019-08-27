/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

#include "em_gpio.h"
#include "em_cmu.h"

#include "xsmc.h"
#include "xsHost.h"
#include "xsPlatform.h"

#include "mc.defines.h"

#include "em_device.h"
#include "em_usart.h"
#include "em_leuart.h"
#include "em_system.h"

#include "modGPIO.h"

#include "../modSerial.h"

extern uint32_t msTickCount;

typedef struct modSerialDeviceRecord {
#if defined(MODDEF_SERIAL_INTERFACE_LEUART)
	LEUART_TypeDef	*uart;
	LEUART_Init_TypeDef serialInit;
#else
	USART_TypeDef	*uart;
	USART_InitAsync_TypeDef serialInit;
#endif
	int				timeoutMS;
	int				timeoutEndMS;

	// for poll (or interrupt)
//	char 			terminators[MAX_TERMINATORS];
	int				numTerminators;
	uint8_t			trim;
	modSerialPollCallbackFn	poll_callback;
	uint8_t			*data;
	int				dataLen;
	int				dataPos;
	void			*refcon;
} modSerialDeviceRecord, *modSerialDevice;

static int gSerialSetup = 0;
modSerialDevice gSerial = NULL;

static char gTerminatorCheck[MAX_TERMINATORS];
static int gNumTerminators = 0;
static int issuedCallback = 0;

void modSerialGotTerminator(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

#ifndef MODDEF_SERIAL_TX_LOCATION
	#define MODDEF_SERIAL_TX_LOCATION MODDEF_SERIAL_LOCATION
#endif
#ifndef MODDEF_SERIAL_RX_LOCATION
	#define MODDEF_SERIAL_RX_LOCATION MODDEF_SERIAL_LOCATION
#endif
#ifndef MODDEF_SERIAL_RTS_LOCATION
	#define MODDEF_SERIAL_RTS_LOCATION MODDEF_SERIAL_LOCATION
#endif

#if defined(MODDEF_SERIAL_INTERFACE_LEUART)
	#if (MODDEF_SERIAL_INTERFACE_LEUART == 0)
		#define SERIAL_PORT		LEUART0
		#define SERIAL_CLOCK	cmuClock_LEUART0
		#define SERIAL_INIT_CLOCK() CMU_ClockEnable(cmuClock_CORELE, true); CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO); CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1); CMU_ClockEnable(cmuClock_LEUART0, true);

	#elif (MODDEF_SERIAL_INTERFACE_LEUART == 1)
		#define SERIAL_PORT		LEUART1
		#define SERIAL_CLOCK	cmuClock_LEUART1
		#define SERIAL_INIT_CLOCK() CMU->LFCLKSEL &= ~_CMU_LFCLKSEL_LFB_MASK; CMU->LFCLKSEL |= CMU_LFCLKSEL_LFBE_ULFRCO; CMU->LFBCLKEN0 |= CMU_LFBCLKEN0_LEUART1;
	#endif
#elif defined(MODDEF_SERIAL_INTERFACE_UART)
	#if (MODDEF_SERIAL_INTERFACE_UART == 0)
		#define SERIAL_PORT		UART0
		#define SERIAL_CLOCK	cmuClock_UART0
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_UART0;
		#define SERIAL_RX_IRQ	UART0_RX_IRQn
		#define SERIAL_TX_IRQ	UART0_TX_IRQn
	#elif (MODDEF_SERIAL_INTERFACE_UART == 1)
		#define SERIAL_PORT		UART1
		#define SERIAL_CLOCK	cmuClock_UART1
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_UART1;
		#define SERIAL_RX_IRQ	UART1_RX_IRQn
		#define SERIAL_TX_IRQ	UART1_TX_IRQn
	#endif
#elif defined(MODDEF_SERIAL_INTERFACE_USART)
	#if (MODDEF_SERIAL_INTERFACE_USART == 0)
		#define SERIAL_PORT		USART0
		#define SERIAL_CLOCK	cmuClock_USART0
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_ULFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART0;
		#define SERIAL_RX_IRQ	USART0_RX_IRQn
		#define SERIAL_TX_IRQ	USART0_TX_IRQn
	#elif (MODDEF_SERIAL_INTERFACE_USART == 1)
		#define SERIAL_PORT		USART1
		#define SERIAL_CLOCK	cmuClock_USART1
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_ULFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1;
		#define SERIAL_RX_IRQ	USART1_RX_IRQn
		#define SERIAL_TX_IRQ	USART1_TX_IRQn
	#elif (MODDEF_SERIAL_INTERFACE_USART == 2)
		#define SERIAL_PORT		USART2
		#define SERIAL_CLOCK	cmuClock_USART2
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_ULFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART2;
		#define SERIAL_RX_IRQ	USART2_RX_IRQn
		#define SERIAL_TX_IRQ	USART2_TX_IRQn
	#elif (MODDEF_SERIAL_INTERFACE_USART == 3)
		#define SERIAL_PORT		USART3
		#define SERIAL_CLOCK	cmuClock_USART3
		#define SERIAL_INIT_CLOCK() CMU_ClockSelectSet(cmuClock_HF, cmuSelect_ULFRCO); CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART3;
		#define SERIAL_RX_IRQ	USART3_RX_IRQn
		#define SERIAL_TX_IRQ	USART3_TX_IRQn
	#endif
#else
	#error unknown serial interface
#endif

#define BUFFERSIZE	256
volatile struct circBuf {
	uint8_t		data[BUFFERSIZE];
	uint16_t	readIdx;
	uint16_t	writeIdx;
	uint16_t	pendingBytes;
	bool		overflow;
} serialRxBuf, serialTxBuf = { {0}, 0, 0, 0, false };

int setMaxSleep(int sleepLevel);
static int modLastSleep = 3;

#define blockInterrupts() { NVIC_DisableIRQ(SERIAL_RX_IRQ); NVIC_DisableIRQ(SERIAL_TX_IRQ); }
#define blockRxInterrupts() { NVIC_DisableIRQ(SERIAL_RX_IRQ); }
#define blockTxInterrupts() { NVIC_DisableIRQ(SERIAL_TX_IRQ); }
#define unblockInterrupts() { NVIC_EnableIRQ(SERIAL_RX_IRQ); NVIC_EnableIRQ(SERIAL_TX_IRQ); }
#define unblockRxInterrupts() { NVIC_EnableIRQ(SERIAL_RX_IRQ); }
#define unblockTxInterrupts() { NVIC_EnableIRQ(SERIAL_TX_IRQ); }

#if defined(MODDEF_SERIAL_INTERFACE_UART) && (MODDEF_SERIAL_INTERFACE_UART == 0)
void UART0_RX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_UART == 1)
void UART1_RX_IRQHandler(void)
#elif defined(MODDEF_SERIAL_INTERFACE_USART) && (MODDEF_SERIAL_INTERFACE_USART == 0)
void USART0_RX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 1)
void USART1_RX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 2)
void USART2_RX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 3)
void USART3_RX_IRQHandler(void)
#else
	#error Need to implement IRQ Handler
#endif
{
	if (SERIAL_PORT->STATUS & USART_STATUS_RXDATAV) {
		uint8_t rxData = USART_Rx(SERIAL_PORT);

		serialRxBuf.data[serialRxBuf.writeIdx] = rxData;
		serialRxBuf.writeIdx = (serialRxBuf.writeIdx + 1) % BUFFERSIZE;

		if (serialRxBuf.pendingBytes == BUFFERSIZE)
			serialRxBuf.overflow = true;
		else
			serialRxBuf.pendingBytes++;

		if (!issuedCallback && gSerial && gNumTerminators) {
			int j;
			for (j=0; j<gNumTerminators; j++) {
				if (rxData == gTerminatorCheck[j]) {
					issuedCallback = 1;
					modMessagePostToMachineFromPool(NULL, modSerialGotTerminator, gSerial);
				}
			}
		}

		USART_IntClear(SERIAL_PORT, USART_IF_RXDATAV);
	}
}

#if defined(MODDEF_SERIAL_INTERFACE_UART) && (MODDEF_SERIAL_INTERFACE_UART == 0)
void UART0_TX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_UART == 1)
void UART1_TX_IRQHandler(void)
#elif defined(MODDEF_SERIAL_INTERFACE_USART) && (MODDEF_SERIAL_INTERFACE_USART == 0)
void USART0_TX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 1)
void USART1_TX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 2)
void USART2_TX_IRQHandler(void)
#elif (MODDEF_SERIAL_INTERFACE_USART == 3)
void USART3_TX_IRQHandler(void)
#else
	#error Need to implement IRQ Handler
#endif
{
	USART_IntGet(SERIAL_PORT);

	if (SERIAL_PORT->STATUS & USART_STATUS_TXBL) {
		if (serialTxBuf.pendingBytes > 0) {
			USART_Tx(SERIAL_PORT, serialTxBuf.data[serialTxBuf.readIdx]);
			serialTxBuf.readIdx = (serialTxBuf.readIdx + 1) % BUFFERSIZE;
			serialTxBuf.pendingBytes --;
		}
		if (serialTxBuf.pendingBytes == 0)
			USART_IntDisable(SERIAL_PORT, USART_IF_TXBL);
	}

}

void modSerialGotTerminator(void *the, void *refcon, uint8_t *message, uint16_t messageLength) {
	int c, j;
	modSerialDevice serial = (modSerialDevice)refcon;

	issuedCallback = 0;
	while (-1 != (c = modSerial_getc(serial))) {
		*(serial->data + serial->dataPos) = c;
		serial->dataPos++;
		for (j=0; j<gNumTerminators; j++) {
			if (c == gTerminatorCheck[j]) {
				// got a terminator
				if (serial->trim) {
					serial->dataPos--;
					*(serial->data + serial->dataPos) = '\0';
				}

				// make the callback
				if (serial->dataPos > 0)
					(serial->poll_callback)(serial->data, serial->dataPos, serial->refcon);

				serial->dataPos = 0;
			}
		}
		if (serial->dataPos == serial->dataLen) {
			// hit the end.
			// make the callback
			(serial->poll_callback)(serial->data, serial->dataPos, serial->refcon);
			serial->dataPos = 0;
		}
	}
}

void modSerial_poll(modSerialDevice serial, int interval, char *terminators, uint8_t trim, uint8_t *data, int dataLen, modSerialPollCallbackFn callback, void *refcon) {
	c_memcpy(gTerminatorCheck, terminators, 16);
	gNumTerminators = c_strlen(terminators);
	serial->trim = trim;
	serial->data = data;
	serial->dataPos = 0;
	serial->dataLen = dataLen;
	serial->poll_callback = callback;
	serial->refcon = refcon;

	// call check once if there is data in buffer
	if (modSerial_available(serial))
		modSerialGotTerminator(NULL, serial, NULL, 0);
}

void modSerial_endPoll(modSerialDevice serial) {
	gNumTerminators = 0;
}

void yield() {
	modDelayMilliseconds(1);
}

void modSerial_puts(modSerialDevice serial, uint8_t *buf, int len) {
	while (serialTxBuf.pendingBytes + len >= BUFFERSIZE) {
		/* wait for space */
		yield();
	}

	blockTxInterrupts();
	while (len--) {
		serialTxBuf.data[serialTxBuf.writeIdx] = *buf++;
		serialTxBuf.writeIdx = (serialTxBuf.writeIdx + 1) % BUFFERSIZE;
		serialTxBuf.pendingBytes++;
	}
	unblockTxInterrupts();

	USART_IntEnable(SERIAL_PORT, USART_IF_TXBL);
}

void modSerial_putc(modSerialDevice serial, int c) {
	while (serialTxBuf.pendingBytes >= BUFFERSIZE) {
		/* wait for space */
		yield();
	}

	blockTxInterrupts();
	serialTxBuf.data[serialTxBuf.writeIdx] = c;
	serialTxBuf.writeIdx = (serialTxBuf.writeIdx + 1) % BUFFERSIZE;
	serialTxBuf.pendingBytes++;
	unblockTxInterrupts();

	USART_IntEnable(SERIAL_PORT, USART_IF_TXBL);
}

int modSerial_gets(modSerialDevice serial, uint8_t *buf, int len) {
	int num = 0;
	blockRxInterrupts();
	while (len--) {
		num++;
		*buf++ = serialRxBuf.data[serialRxBuf.readIdx];
		serialRxBuf.readIdx = (serialRxBuf.readIdx + 1) % BUFFERSIZE;
		serialRxBuf.pendingBytes--;
	}
	unblockRxInterrupts();
	return num;
}

int modSerial_getc(modSerialDevice serial) {
	uint8_t ch;
	if (serialRxBuf.pendingBytes < 1)
		return -1;
	blockRxInterrupts();
	ch = serialRxBuf.data[serialRxBuf.readIdx];
	serialRxBuf.readIdx = (serialRxBuf.readIdx + 1) % BUFFERSIZE;
	serialRxBuf.pendingBytes--;
	unblockRxInterrupts();

	return ch;
}


void modSerial_setTimeout(modSerialDevice serial, int timeoutMS) {
	serial->timeoutMS = timeoutMS;
}

int modSerial_read(modSerialDevice serial, uint8_t *buf, int len) {
	int amt = len;

	serial->timeoutEndMS = msTickCount + serial->timeoutMS;
	
	blockRxInterrupts();
	while (amt) {
		if (msTickCount > serial->timeoutEndMS)
			break;
		if (serialRxBuf.pendingBytes < 1) {
			unblockRxInterrupts();
			yield();
			blockRxInterrupts();
			continue;
		}
		*buf++ = serialRxBuf.data[serialRxBuf.readIdx];
		serialRxBuf.readIdx = (serialRxBuf.readIdx + 1) % BUFFERSIZE;
		serialRxBuf.pendingBytes--;
		amt--;
	}
	unblockRxInterrupts();

	return len - amt;
}

int modSerial_write(modSerialDevice serial, uint8_t *buf, int len) {
	int amt = len;

	serial->timeoutEndMS = msTickCount + serial->timeoutMS;
	
	blockTxInterrupts();
	while (amt) {
		if (msTickCount > serial->timeoutEndMS)
			break;
		if (serialTxBuf.pendingBytes == BUFFERSIZE) {
			unblockTxInterrupts();
			yield();
			blockTxInterrupts();
			continue;
		}
		serialTxBuf.data[serialTxBuf.writeIdx] = *buf++;
		serialTxBuf.writeIdx = (serialTxBuf.writeIdx + 1) % BUFFERSIZE;
		serialTxBuf.pendingBytes++;
		amt--;
	}
	unblockTxInterrupts();

	USART_IntEnable(SERIAL_PORT, USART_IF_TXBL);

	return len - amt;
}


void modSerial_flush(modSerialDevice serial) {
	blockInterrupts();
	serialRxBuf.readIdx = serialRxBuf.writeIdx = serialRxBuf.pendingBytes = serialRxBuf.overflow = 0;
	serialTxBuf.readIdx = serialTxBuf.writeIdx = serialTxBuf.pendingBytes = serialTxBuf.overflow = 0;
	unblockInterrupts();
}

int modSerial_available(modSerialDevice serial) {
	return serialRxBuf.pendingBytes;
}

void modSerial_setBaudrate(modSerialDevice serial, int speed) {
}

int modSerial_peek(modSerialDevice serial) {
	if (serialRxBuf.pendingBytes > 0)
		return -1;
	return serialRxBuf.data[serialRxBuf.readIdx];
}

int modSerial_txBusy(modSerialDevice serial) {
	return serialTxBuf.pendingBytes;
}

void modSerial_teardown(modSerialDevice serial) {
	setMaxSleep(modLastSleep);
	gSerialSetup = 0;
	gNumTerminators = 0;
	c_free(serial);
	gSerial = NULL;
}

modSerialDevice modSerialDevice_setup(modSerialConfig config) {
	modSerialDevice serial;

	serial = c_calloc(1, sizeof(modSerialDeviceRecord));

#if MIGHTY_GECKO || BLUE_GECKO || THUNDERBOARD2
	CMU_ClockEnable(SERIAL_CLOCK, true);

	GPIO_PinModeSet(config->txPort, config->txPin, gpioModePushPull, 1);
	GPIO_PinModeSet(config->rxPort, config->rxPin, gpioModeInput, 0);

	serial->serialInit.enable = usartDisable;
	serial->serialInit.mvdis = false;		// don't disable majority vote
	serial->serialInit.oversampling = usartOVS16;
	serial->serialInit.prsRxCh = usartPrsRxCh0;
	serial->serialInit.prsRxEnable = false;
	serial->serialInit.refFreq = 0;

	serial->serialInit.baudrate = config->baud;

	if (config->databits == 5)
		serial->serialInit.databits = usartDatabits5;
	else if (config->databits == 6)
		serial->serialInit.databits = usartDatabits6;
	else if (config->databits == 7)
		serial->serialInit.databits = usartDatabits7;
	else if (config->databits == 9)
		serial->serialInit.databits = usartDatabits9;
	else if (config->databits == 10)
		serial->serialInit.databits = usartDatabits10;
	else if (config->databits == 11)
		serial->serialInit.databits = usartDatabits11;
	else
		serial->serialInit.databits = usartDatabits8;

	if (config->parity[0] == 'E' || config->parity[0] == 'e') {
		serial->serialInit.parity = usartEvenParity;
	}
	else if (config->parity[0] == 'O' || config->parity[0] == 'o') {
		serial->serialInit.parity = usartOddParity;
	}
	else
		serial->serialInit.parity = usartNoParity;

	if (config->stopbits == 5) {
		serial->serialInit.stopbits = usartStopbits0p5;
	}
	else if (config->stopbits == 15) {
		serial->serialInit.stopbits = usartStopbits1p5;
	}
	else if (config->stopbits == 2) {
		serial->serialInit.stopbits = usartStopbits2;
	}
	else {
		serial->serialInit.stopbits = usartStopbits1;
	}

	USART_InitAsync(SERIAL_PORT, &serial->serialInit);

	SERIAL_PORT->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
	SERIAL_PORT->ROUTELOC0  = ( SERIAL_PORT->ROUTELOC0 & ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK ) );
	SERIAL_PORT->ROUTELOC0 |= ( MODDEF_SERIAL_TX_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT );
	SERIAL_PORT->ROUTELOC0 |= ( MODDEF_SERIAL_RX_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT );

	USART_IntClear(SERIAL_PORT, _USART_IF_MASK);
	USART_IntEnable(SERIAL_PORT, _USART_IF_RXDATAV_MASK);
	NVIC_ClearPendingIRQ(SERIAL_RX_IRQ);
	NVIC_ClearPendingIRQ(SERIAL_TX_IRQ);
	NVIC_EnableIRQ(SERIAL_RX_IRQ);
	NVIC_EnableIRQ(SERIAL_TX_IRQ);

	USART_Enable(SERIAL_PORT, usartEnable);

#elif GIANT_GECKO

	CMU_ClockEnable(SERIAL_CLOCK, true);
	SERIAL_INIT_CLOCK();

	GPIO_PinModeSet(MODDEF_SERIAL_TX_PORT, MODDEF_SERIAL_TX_PIN, gpioModePushPull, 1);
	GPIO_PinModeSet(MODDEF_SERIAL_RX_PORT, MODDEF_SERIAL_RX_PIN, gpioModeInput, 0);

#if defined(MODDEF_SERIAL_INTERFACE_LEUART)
	serialInit = LEUART_INIT_DEFAULT;
	serialInit.baudrate = MODDEF_SERIAL_BAUD;
	LEUART_Init(SERIAL_PORT, &serialInit);
#elif defined(MODDEF_SERIAL_INTERFACE_UART) || defined(MODDEF_SERIAL_INTERFACE_USART)
	USART_InitAsync_TypeDef serialInit = USART_INITASYNC_DEFAULT;

	serialInit.enable = usartDisable;
	serialInit.baudrate = MODDEF_SERIAL_BAUD;
	USART_InitAsync(SERIAL_PORT, &serialInit);
#else
	modLog("unknown serial PORT");
#endif

	SERIAL_PORT->ROUTE = (MODDEF_SERIAL_LOCATION << _USART_ROUTE_LOCATION_SHIFT)
			| USART_ROUTE_RXPEN | USART_ROUTE_TXPEN;

	gSerialSetup = 1;

	USART_IntClear(SERIAL_PORT, _UART_IF_MASK);
	USART_IntEnable(SERIAL_PORT, UART_IF_RXDATAV);
	NVIC_ClearPendingIRQ(SERIAL_RX_IRQ);
	NVIC_ClearPendingIRQ(SERIAL_TX_IRQ);
	NVIC_EnableIRQ(SERIAL_RX_IRQ);
	NVIC_EnableIRQ(SERIAL_TX_IRQ);

	USART_Enable(SERIAL_PORT, usartEnable);
#else
	#error undefined gecko platform
#endif

	modLastSleep = setMaxSleep(1);

	gSerial = serial;
	return serial;
}

