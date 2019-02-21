/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xsmc.h"
#include "xsgecko.h"
#include "xsPlatform.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "em_core.h"

#include "mc.xs.h"      // for xsID_ values

#define ID(symbol) (xsID_##symbol)

#if 1
#define serialUSARTPort gpioPortB
#define serialUSARTTxPin	6			// EXP Header pin 12 - US3_TX#10
#define serialUSARTRxPin	7			// EXP Header pin 14 - US3_RX#10
#define SERIAL_USART		USART3
#define SERIAL_CLOCK	cmuClock_USART3
#define SERIAL_USARTLOC		10
#define SERIAL_TX_IRQ		USART3_TX_IRQn
#define SERIAL_RX_IRQ		USART3_RX_IRQn
#else
#define serialUSARTPort gpioPortD
#define serialUSARTTxPin	11			// EXP Header pin 9 - US1_TX#19
#define serialUSARTRxPin	12			// EXP Header pin 11 - US1_RX#19
#define SERIAL_USART		USART1
#define SERIAL_CLOCK	cmuClock_USART1
#define SERIAL_USARTLOC		19
#define SERIAL_TX_IRQ		USART1_TX_IRQn
#define SERIAL_RX_IRQ		USART1_RX_IRQn
#endif

#define SEND_BUFFER_SIZE	256
typedef struct {
	uint16_t head;
	uint16_t tail;
	uint8_t data[SEND_BUFFER_SIZE];
	bool processing;
} serialBufferRec;

serialBufferRec send_buffer = { .head=0, .tail=0, .data=0, .processing=false }; 
serialBufferRec recv_buffer = { .head=0, .tail=0, .data=0, .processing=false }; 
#define dontOverflow(i)  ((i>=SEND_BUFFER_SIZE) ? 0 : i)

void xs_Serial(xsMachine *the) {
//	gRadioMachine = the;
	USART_InitAsync_TypeDef serialInit = USART_INITASYNC_DEFAULT;
    CMU_ClockEnable(SERIAL_CLOCK, true);

	GPIO_PinModeSet(serialUSARTPort, serialUSARTTxPin, gpioModePushPull, 1);
	GPIO_PinModeSet(serialUSARTPort, serialUSARTRxPin, gpioModeInput, 0);

	serialInit.baudrate = 9600;

    USART_InitAsync(SERIAL_USART, &serialInit);

    SERIAL_USART->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
    SERIAL_USART->ROUTELOC0  = ( SERIAL_USART->ROUTELOC0 & ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK ) );
    SERIAL_USART->ROUTELOC0 |= ( SERIAL_USARTLOC << _USART_ROUTELOC0_TXLOC_SHIFT );
    SERIAL_USART->ROUTELOC0 |= ( SERIAL_USARTLOC << _USART_ROUTELOC0_RXLOC_SHIFT );

	USART_IntClear(SERIAL_USART, USART_IF_RXDATAV);
	USART_IntEnable(SERIAL_USART, USART_IF_RXDATAV);
	NVIC_ClearPendingIRQ(SERIAL_RX_IRQ);
	NVIC_EnableIRQ(SERIAL_RX_IRQ);

	USART_IntClear(SERIAL_USART, USART_IF_TXC);
	USART_IntEnable(SERIAL_USART, USART_IF_TXC);
	NVIC_ClearPendingIRQ(SERIAL_TX_IRQ);
	NVIC_EnableIRQ(SERIAL_TX_IRQ);
}

void xs_Serial_destructor(void *data) {
}

void xs_Serial_setBaudrate(xsMachine *the) {
	uint32_t baud = xsmcToInteger(xsArg(0));
	USART_BaudrateAsyncSet(SERIAL_USART, 0, baud, usartOVS16);
}

static uint8_t serialIsReadable() {
	if (SERIAL_USART->STATUS & (1<<7))
		return 1;
	return 0;
}

static int serialGetChar(void) {
	if (!serialIsReadable())
		return -1;
	return SERIAL_USART->RXDATA;
}

static void serialPutChar(char *c) {
	while (!(SERIAL_USART->STATUS & (1 << 6)) );  // wait for TX buffer to empty
	SERIAL_USART->TXDATA = c;					// send character
}

//void USART3_TX_IRQHandler(void) {
void USART3_TX_IRQHandler(void) {
	if (SERIAL_USART->IF && USART_IF_TXC) {
		USART_IntClear(SERIAL_USART, USART_IF_TXC);
		if (send_buffer.tail != send_buffer.head) {
			USART_Tx(SERIAL_USART, send_buffer.data[send_buffer.tail++]);
			send_buffer.tail = dontOverflow(send_buffer.tail);
		}
		else
			send_buffer.processing = false;
	}
}

void USART3_RX_IRQHandler(void) {
	if (SERIAL_USART->IF && USART_IF_RXDATAV) {
//		USART_IntClear(SERIAL_USART, USART_IF_RXDATAV);
		if (dontOverflow(recv_buffer.head+1) != recv_buffer.tail) {
			if (SERIAL_USART->STATUS & (1<<7)) {
				recv_buffer.data[recv_buffer.head++] = SERIAL_USART->RXDATA; // USART_Rx(SERIAL_USART);
				recv_buffer.head = dontOverflow(recv_buffer.head);
			}
		}
	}
}

static void sendSerialString(char *string) {
	while (*string) {
		send_buffer.data[send_buffer.head++] = *string++;
		send_buffer.head = dontOverflow(send_buffer.head);
	}
    NVIC_DisableIRQ(SERIAL_TX_IRQ);
	if (!send_buffer.processing) {
		send_buffer.processing = true;
		USART_Tx(SERIAL_USART, send_buffer.data[send_buffer.tail++]);
		send_buffer.tail = dontOverflow(send_buffer.tail);
	}
    NVIC_EnableIRQ(SERIAL_TX_IRQ);
}

void xs_Serial_send(xsMachine *the) {
	int argc = xsmcArgc, i;
	for (i=0; i<argc; i++) {
		char *str = xsmcToString(xsArg(i));
#if 1
		sendSerialString(str);
#else
		do {
			uint8_t c = c_read8(str);
			if (!c) break;
			serialPutChar(c);
			str++;
		} while (true);
#endif
	}
}

void xs_Serial_receive(xsMachine *the) {
#if 1
	if (recv_buffer.tail != recv_buffer.head) {
		xsmcSetInteger(xsResult, recv_buffer.data[recv_buffer.tail++]);
		recv_buffer.tail = dontOverflow(recv_buffer.tail);
	}
#else
	int c = serialGetChar();
	if (-1 == c)
		return;
	xsmcSetInteger(xsResult, c);
#endif
}

