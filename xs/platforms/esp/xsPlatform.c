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

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void *)0x87654321)

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
	the->debugOnReceive = true;
#endif
#if !ESP32
	init_printf(the, fx_putc);
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
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
			static const char *xsbugHeader = "\r\n<xsbug><log>";
			const char *cp = xsbugHeader;
			while (true) {
				char c = c_read8(cp++);
				if (!c) break;
				ESP_putc(c);
			}
		}
	}

	ESP_putc(c);
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

uint8_t triggerDebugCommand(txMachine *the)
{
	if (the->debugOnReceive) {
		fxDebugCommand(the);
		return the->breakOnStartFlag;
	}

	return false;
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

	if (the->context) {
		uint8_t **context = the->context;
		if (context[2])
			return;
	}

	if (isSerialIP(gXSBUG)) {
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
	if (kSerialConnection == the->connection)
		return (txBoolean)ESP_isReadable();
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
		while (the->debugOffset < (sizeof(the->debugBuffer) - 3)) {
			int c = ESP_getc();
			if (-1 == c) {
				modDelayMilliseconds(2);
				c = ESP_getc();
				if (-1 == c)
					break;
			}

			the->debugBuffer[the->debugOffset++] = (txU1)c;
		}
	}

bail:
	the->debugBuffer[the->debugOffset] = 0;

	xmodLog("  fxReceive - EXIT with:");
	xmodLog(the->debugBuffer);
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
		while (count--)
			ESP_putc(*c++);
	}

	xmodLog("  fxSend - EXIT");
}

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

