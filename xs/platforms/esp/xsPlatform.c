/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	the->debugOnReceive = true;
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
		xmodLog("  fxConnect - didConnect - OK");
		the->connection = tpcb;
	}

	return ERR_OK;
}

static err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	txMachine* the = arg;

    xmodLog("  didReceive - ENTER");

	if (NULL == p) {
		modLog("  didReceive - CONNECTION CLOSED");
		return ERR_OK;
	}

	if (NULL != the->reader) {
		modLog("  UNEXPECTED NETWORK PACKET");
		pbuf_free(the->reader);
		the->reader = NULL;		// unprocessed command. free the buffer so it doesn't linger forever.
	}

	the->reader = p;

    xmodLog("  didReceive - EXIT");

	return ERR_OK;
}

static err_t didReceiveMonitor(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	txMachine* the = arg;
	err_t result = didReceive(arg, pcb, p, err);

	if (the->debugOnReceive && (ERR_OK == result) && the->reader) {
//		ESP_REPORT("  didReceiveMonitor - fxDebugCommand");
//		fxDebugCommand(the);
//@@ rather than call fxDebugCommand here, use an immediate timer to defer to main loop
//@@ otherwise, we can end up with (unexpected) re-entrant calls
	}

	return result;
}

static err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	txMachine* the = arg;
	the->pendingSendBytes -= len;
	return ERR_OK;
}

static void didError(void *arg, err_t err)
{
	modLog("XSBUG SOCKET ERROR!!!!!");
	modLogInt(err);
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

    xmodLog("  fxConnect - ENTER");

    xmodLog("  fxConnect - call tcp_new");
	pcb = tcp_new();
	the->reader = NULL;

    xmodLog("  fxConnect - call tcp_bind");
	err = tcp_bind(pcb, IP_ADDR_ANY, 0);
	if (err) modLog("  fxConnect - tcp_bind ERROR");

	xmodLog("  fxConnect - make address - 192.168.1.30");
#if ESP32
	IP4_ADDR(ip_2_ip4(&ipaddr), gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#else
	IP4_ADDR(&ipaddr, gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#endif

	tcp_arg(pcb, the);

    modLog("  fxConnect - connect to XSBUG");
	err = tcp_connect(pcb, &ipaddr, 5002, didConnect);
	if (err) {
		modLog("  fxConnect - tcp_connect ERROR");
		return;
	}

	while (!the->connection)
		modDelayMilliseconds(100);

	if ((txSocket)-1 == the->connection) {
		the->connection = NULL;
		modLog("  fxConnect - couldn't connect");
	}

	tcp_recv(pcb, didReceiveMonitor);
	tcp_sent(pcb, didSend);
	tcp_err(pcb, didError);
	tcp_nagle_disable(pcb);

    xmodLog("  fxConnect - EXIT");
}

void fxDisconnect(txMachine* the)
{
	xmodLog("  fxDisconnect - ENTER");
	if (the->connection) {
		if (kSerialConnection != the->connection)
			tcp_close((struct tcp_pcb *)the->connection);
		else
			fx_putpi(the, '-', true);
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
	else
		return the->connection && the->reader;
}

void fxReceive(txMachine* the)
{
	struct tcp_pcb *pcb = the->connection;

	xmodLog("  fxReceive - ENTER");

	the->debugOffset = 0;

	if (!pcb) return;

	if (kSerialConnection != pcb) {
		struct pbuf *p;

		while (NULL == the->reader)
			modDelayMilliseconds(100);

		p = the->reader;
		if (p->next)
			modLog("  fxReceive - MULTIPLE PACKETS!!");

		if (p->len >=  sizeof(the->debugBuffer)) {
			modLog("  fxReceive - more than we can handle!!!");
			goto bail;
		}
		c_memmove(the->debugBuffer, p->payload, p->len);
		the->debugOffset = p->len;
		the->debugBuffer[the->debugOffset] = 0;		// note: pin input to debugSize - 1 to leave room for this byte

		tcp_recved(pcb, p->tot_len);
		pbuf_free(the->reader);
		the->reader = NULL;
	}
	else {
		while (true) {
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
	}

bail:
	the->debugBuffer[the->debugOffset] = 0;

	xmodLog("  fxReceive - EXIT with:");
	if (the->debugOffset)
		xmodLogVar(the->debugBuffer);
}

static void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	txMachine* the = machine;

	the->debugNotifyOutstanding = false;

	if (!the->debugFragments || !the->debugOnReceive)
		return;

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
	static uint8_t buffered[28];
	static uint8_t bufferedBytes = 0;
	txBoolean didRead = false;

	mxDebugMutexTake();

	while (true) {
		int c = ESP_getc();
		if (-1 == c) {
			if (!didRead)
				break;
			int retry;
			for (retry = 0; retry < 5; retry++) {
				modDelayMilliseconds(1);
				c = ESP_getc();
				if (-1 != c)
					break;
			}
			if (-1 == c) break;
		}
		didRead = true;

		if ((state >= 0) && (state <= 6)) {
			if (0 == state) {
				current = NULL;
				value = 0;
			}
			if (c == piBegin[state])
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
			if (c == tagEnd[state - 17]) {
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
				modDelayMilliseconds(100);
				continue;
			}

			if (available > length)
				available = length;

			while (true) {
				the->pendingSendBytes += available;
				// symchronous write so copy flag is unnecessary
				err = tcp_write(pcb, bytes, available, (more && (available == length)) ? TCP_WRITE_FLAG_MORE : 0);
				if (ERR_MEM == err) {
					the->pendingSendBytes -= available;
					modLog("  fxSend - wait for send memory:");
					tcp_output(pcb);
					modDelayMilliseconds(100);
					continue;
				}
				if (err) {
					the->pendingSendBytes -= available;
					modLog("  fxSend - tcp_write ERROR:");
					modLogInt(err);
				}
				break;
			}

			while (the->pendingSendBytes) {
				err = tcp_output(pcb);
				if (err)
					modLog("  fxSend - tcp_output ERROR:");
				modDelayMilliseconds(10);
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

