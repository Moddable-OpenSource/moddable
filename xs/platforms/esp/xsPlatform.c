/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsAll.h"
#include "stdio.h"
#include "lwip/tcp.h"

#if ESP32
	#include "rom/ets_sys.h"
#else
	#include "tinyprintf.h"
#endif

#include "xsesp.h"

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void *)0x87654321)

static void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf);

#if defined (mxDebug) && ESP32
	SemaphoreHandle_t gDebugMutex;

	#define mxDebugMutexTake() xSemaphoreTake(gDebugMutex, portMAX_DELAY)
	#define mxDebugMutexGive() xSemaphoreGive(gDebugMutex)

	static int fx_vprintf(const char *str, va_list list);
#else
	#define mxDebugMutexTake()
	#define mxDebugMutexGive()
#endif

void fxCreateMachinePlatform(txMachine* the)
{
	modMachineTaskInit(the);
#ifdef mxDebug
	the->connection = mxNoSocket;
#if ESP32
	if (!gDebugMutex) {
		gDebugMutex = xSemaphoreCreateMutex();
		esp_log_set_vprintf(fx_vprintf);
	}
#endif
#endif
#if !ESP32
	init_printf(the, fx_putc);
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
	while (the->debugFragments) {
		DebugFragment next = the->debugFragments->next;
		c_free(the->debugFragments);
		the->debugFragments = next;
	}

	modMachineTaskUninit(the);
}

void fx_putc(void *refcon, char c)
{
	txMachine* the = refcon;

	if (the->inPrintf) {
		if (0 == c) {
			if (kSerialConnection == the->connection) {
				// write xsbug log trailer
				txU1 stop, state;
				const static const char *xsbugTrailer = "&#10;</log></xsbug>\r\n";
				const char *cp = xsbugTrailer;
				while (true) {
					char c = c_read8(cp++);
					if (!c) break;
					ESP_putc(c);
				}
			}
			the->inPrintf = false;
			return;
		}
	}
	else {
		if (0 == c)
			return;

		the->inPrintf = true;
		if (kSerialConnection == the->connection) {
			// write xsbug log header
			static const char *xsbugHeader = "<xsbug><log>";
			const char *cp = xsbugHeader;
			fx_putpi(the, '.', true);
			while (true) {
				char c = c_read8(cp++);
				if (!c) break;
				ESP_putc(c);
			}
		}
	}

	ESP_putc(c);
}

void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf)
{
	static const char *xsbugHeaderStart = "\r\n<?xs";
	static const char *xsbugHeaderEnd = "?>";
	static const char *gHex = "0123456789ABCDEF";
	char hex[10];
	signed char i;
	const char *cp = xsbugHeaderStart;
	while (true) {
		char c = c_read8(cp++);
		if (!c) break;
		ESP_putc(c);
	}

	ESP_putc(separator);

	for (i = 7; i >= 0; i--)
		ESP_putc(c_read8(gHex + ((((uintptr_t)the) >> (i << 2)) & 0x0F)));

	cp = xsbugHeaderEnd;
	while (true) {
		char c = c_read8(cp++);
		if (!c) break;
		ESP_putc(c);
	}

	if (trailingcrlf) {
		ESP_putc('\r');
		ESP_putc('\n');
	}
}

#ifdef mxDebug

static void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength);

void fxAbort(txMachine* the)
{
	fxDisconnect(the);
	c_exit(0);
}

static err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err)
{
	txMachine* the = arg;

	if (err) {
		modLog("  fxConnect - didConnect - ERROR");
		the->connection = (txSocket)-1;
	}
	else {
		modLog("  fxConnect - didConnect - OK");
		the->connection = tpcb;
	}

	return ERR_OK;
}

static err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	txMachine* the = arg;
	uint8_t i;

    xmodLog("  didReceive - ENTER");

	if (NULL == p) {
		modLog("  didReceive - CONNECTION LOST");
		the->connection = NULL;		//@@ fxDisconnect?
		return ERR_OK;
	}

	mxDebugMutexTake();

	for (i = 0; i < kDebugReaderCount; i++) {
		if (NULL == the->readers[i]) {
			the->readers[i] = p;
			p = NULL;
			if (i == 0)
				the->readerOffset = 0;
			break;
		}
	}

	if (NULL == p) {
		if (!the->debugNotifyOutstanding) {
			the->debugNotifyOutstanding = true;
			modMessagePostToMachine(the, NULL, 0, doDebugCommand, the);
		}
	}
	else {
		modLog("  debug receive overflow");
		pbuf_free(p);
	}

	mxDebugMutexGive();

	xmodLog("  didReceive - EXIT");

	return ERR_OK;
}

static void didError(void *arg, err_t err)
{
	txMachine* the = arg;

	modLog("XSBUG SOCKET ERROR!!!!!");
	modLogInt(err);
	the->connection = NULL;
}

void fxConnect(txMachine* the)
{
	struct tcp_pcb *pcb;
	err_t err;
#if ESP32
	ip_addr_t ipaddr;
#else
	struct ip_addr ipaddr;
#endif
	extern unsigned char gXSBUG[4];

	if (isSerialIP(gXSBUG)) {
		static txBoolean once;

		if (!once) {
			static const char *piReset = "<?xs-00000000?>\r\n";
			const char *cp = piReset;

			modDelayMilliseconds(250);

			while (true) {
				char c = c_read8(cp++);
				if (!c) break;
				ESP_putc(c);
			}

			once = true;
		}

		the->connection = kSerialConnection;

		return;
	}

	if (0 == gXSBUG[0]) {
		modLog("  fxConnect - NO XSBUG ADDRESS");
		return;
	}

	modLog("  fxConnect - ENTER");

	pcb = tcp_new();
	if (!pcb) return;

	err = tcp_bind(pcb, IP_ADDR_ANY, 0);
	if (err) return;

#if ESP32
	IP4_ADDR(ip_2_ip4(&ipaddr), gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#else
	IP4_ADDR(&ipaddr, gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#endif

	tcp_arg(pcb, the);
	tcp_recv(pcb, didReceive);
	tcp_err(pcb, didError);
//	tcp_nagle_disable(pcb);

    modLog("  fxConnect - connect to XSBUG");
	err = tcp_connect(pcb, &ipaddr, 5002, didConnect);
	if (err) {
		modLog("  fxConnect - tcp_connect ERROR");
		return;
	}

	while (!the->connection)
		modDelayMilliseconds(10);

	if ((txSocket)-1 == the->connection) {
		the->connection = NULL;
		modLog("  fxConnect - couldn't connect");
	}

    modLog("  fxConnect - EXIT");
}

void fxDisconnect(txMachine* the)
{
	xmodLog("  fxDisconnect - ENTER");
	if (the->connection) {
		if (kSerialConnection != the->connection) {
			uint8_t i;
			for (i = 0; i < kDebugReaderCount; i++) {
				if (the->readers[i]) {
					pbuf_free(the->readers[i]);
					the->readers[i] = NULL;
				}
			}
			the->readerOffset = 0;
			tcp_close((struct tcp_pcb *)the->connection);
			modDelayMilliseconds(100);		// time for FIN packet to be sent
		}
		else {
			fx_putpi(the, '-', true);
			//@@ clear debug fragments?
		}
		the->connection = NULL;
	}
	xmodLog("  fxDisconnect - EXIT");
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (kSerialConnection == the->connection) {
#if !ESP32
		fxReceiveLoop();
#endif
		return NULL != the->debugFragments;
	}
	else {
		extern uint32_t system_get_rtc_time(void);
		static uint32_t next = 0, now;

		if (!the->connection)
			return 0;

		now = system_get_rtc_time();
		if (now > next) {
			next = now + 170 * 1000;		// about 1000 ms
			modDelayMilliseconds(0);		// give network stack a momemt
		}

		return the->readers[0] ? 1 : 0;
	}
}

void fxReceive(txMachine* the)
{
	struct tcp_pcb *pcb = the->connection;
	if (!pcb) return;

	xmodLog("  fxReceive - ENTER");

	if (kSerialConnection != pcb) {
		struct pbuf *p;
		uint16_t use;

		if (NULL == the->readers[0]) {
			modDelayMilliseconds(10);
			return;
		}

		p = the->readers[0];
		if (p->next)
			modLog("  fxReceive - cannot handle fragmented!!");

		mxDebugMutexTake();
			use = p->len - the->readerOffset;
			if (use > (sizeof(the->debugBuffer) - 1))
				use = sizeof(the->debugBuffer) - 1;

			c_memmove(the->debugBuffer, the->readerOffset + (char *)p->payload, use);
			the->debugOffset = use;

			the->readerOffset += use;
			if (the->readerOffset == p->len) {
				uint8_t i;

				tcp_recved(pcb, p->tot_len);
				pbuf_free(p);

				for (i = 0; i < kDebugReaderCount - 1; i++)
					the->readers[i] = the->readers[i + 1];
				the->readers[kDebugReaderCount - 1] = NULL;

				the->readerOffset = 0;
			}
		mxDebugMutexGive();
	}
	else {
		static uint8_t forever = 0;
		uint32_t timeout = forever ? 0 : (modMilliseconds() + 2000);

		while (true) {
			if (timeout && (timeout < modMilliseconds())) {
				fxDisconnect(the);
				break;
			}

			fxReceiveLoop();
			if (the->debugFragments) {
				DebugFragment f;

				mxDebugMutexTake();
				f = the->debugFragments;
				the->debugFragments = f->next;
				mxDebugMutexGive();

				c_memcpy(the->debugBuffer, f->bytes, f->count);
				the->debugOffset = f->count;
				c_free(f);
				break;
			}
		}
		forever = 1;
	}

bail:
	the->debugBuffer[the->debugOffset] = 0;

//	if (the->debugOffset) {
//		modLog("  fxReceive - EXIT with:");
//		modLogVar(the->debugBuffer);
//	}
}

void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	txMachine* the = machine;

	the->debugNotifyOutstanding = false;
	if (kSerialConnection == the->connection) {
		if (!the->debugFragments)
			return;
	}
	else {
		if (!the->readers[0])
			return;
	}

	fxDebugCommand(the);
	if (the->breakOnStartFlag) {
		fxBeginHost(the);
		fxDebugger(the, (char *)__FILE__, __LINE__);
		fxEndHost(the);
	}
}

void fxReceiveLoop(void)
{
	static const char *piBegin = "\r\n<?xs.";
	static const char *tagEnd = ">\r\n";
	static txMachine* current = NULL;
	static uint8_t state = 0;
	static uint32_t value = 0;
	static uint8_t buffered[28];		//@@ this must be smaller than sxMachine / debugBuffer
	static uint8_t bufferedBytes = 0;

	mxDebugMutexTake();

	while (true) {
		int c = ESP_getc();
		if (-1 == c)
			break;

		if ((state >= 0) && (state <= 6)) {
			if (0 == state) {
				current = NULL;
				value = 0;
			}
			if (c == c_read8(piBegin + state))
				state++;
			else
				state = 0;
		}
		else if ((state >= 7) && (state <= 14)) {
			if (('0' <= c) && (c <= '9')) {
				state++;
				value = (value * 16) + (c - '0');
			}
			else if (('a' <= c) && (c <= 'f')) {
				state++;
				value = (value * 16) + (10 + c - 'a');
			}
			else if (('A' <= c) && (c <= 'F')) {
				state++;
				value = (value * 16) + (10 + c - 'A');
			}
			else
				state = 0;
		}
		else if (state == 15) {
			if (c == '?')
				state++; 
			else
				state = 0;
		}
		else if (state == 16) {
			if (c == '>') {
				current = (txMachine*)value;
				state++;
				bufferedBytes = 0;
			}
			else
				state = 0;
		}
		else if ((state >= 17) && (state <= 19)) {
			txBoolean enqueue;
			buffered[bufferedBytes++] = c;
			enqueue = bufferedBytes == sizeof(buffered);
			if (c == c_read8(tagEnd + state - 17)) {
				if (state == 19) {
					state = 0;
					enqueue = true;
				}
				else
					state++;
			}
			else
				state = 17;

			if (enqueue) {
				DebugFragment fragment = c_malloc(sizeof(DebugFragmentRecord) + bufferedBytes);
				if (NULL == fragment) {
					modLog("no fragment memory");
					break;
				}
				fragment->next = NULL;
				fragment->count = bufferedBytes;
				c_memcpy(fragment->bytes, buffered, bufferedBytes);
				if (NULL == current->debugFragments)
					current->debugFragments = fragment;
				else {
					DebugFragment walker = current->debugFragments;
					while (walker->next)
						walker = walker->next;
					walker->next = fragment;
				}
				if (!current->debugNotifyOutstanding) {
					current->debugNotifyOutstanding = true;
					modMessagePostToMachine(current, NULL, 0, doDebugCommand, current);
				}
				bufferedBytes = 0;
			}
		}
	}
	mxDebugMutexGive();
}

void fxSend(txMachine* the, txBoolean more)
{
	struct tcp_pcb *pcb = the->connection;

	xmodLog("  fxSend - ENTER");

	if (!pcb) return;

	if (kSerialConnection != pcb) {
		int length = the->echoOffset;
		const char *bytes = the->echoBuffer;
		err_t err;

		xmodLog("  fxSend - about to loop");

		while (length) {
			u16_t available = tcp_sndbuf(pcb);
			if (0 == available) {
				xmodLog("  fxSend - need to wait");
				tcp_output(pcb);
				modDelayMilliseconds(10);
				continue;
			}

			if (available > length)
				available = length;

			while (true) {
				err = tcp_write(pcb, bytes, available, TCP_WRITE_FLAG_COPY);
				if (ERR_MEM == err) {
					xmodLog("  fxSend - wait for send memory:");
					tcp_output(pcb);
					modDelayMilliseconds(10);
					continue;
				}
				if (err) {
					modLog("  fxSend - tcp_write ERROR:");
					modLogInt(err);
				}
				break;
			}

			length -= available;
			bytes += available;
		}
	}
	else {
		char *c = the->echoBuffer;
		txInteger count = the->echoOffset;
		if (!the->inPrintf) {
			mxDebugMutexTake();
			fx_putpi(the, '.', false);
		}
		the->inPrintf = more;
		while (count--)
			ESP_putc(*c++);

		if (!more)
			mxDebugMutexGive();
	}

	xmodLog("  fxSend - EXIT");
}

#if defined(mxDebug) && ESP32
int fx_vprintf(const char *str, va_list list)
{
	int result;

	mxDebugMutexTake();
		result = vprintf(str, list);
	mxDebugMutexGive();

	return result;
}
#endif

#endif /* mxDebug */

void selectionSort(void *base, size_t num, size_t width, int (*compare )(const void *, const void *))
{
	size_t i, j;
	uint8_t temp[256];

	if (width > sizeof(temp)) {
		modLog("width too big");
		return;
	}

	for (i = 0; i < num - 1; i++) {
		size_t minIndex = i;

		for (j = i + 1; j < num; j++) {
			if (compare((j * width) + (char *)base, (minIndex * width) + (char *)base) < 0)
				minIndex = j;
		}
		if (minIndex == i)
			continue;

		c_memcpy(temp, (i * width) + (char *)base, width);
		c_memcpy((i * width) + (char *)base, (minIndex * width) + (char *)base, width);
		c_memcpy((minIndex * width) + (char *)base, temp, width);
	}
}

