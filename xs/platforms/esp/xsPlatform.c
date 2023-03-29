/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "modTimer.h"
#include "stdio.h"
#include "lwip/tcp.h"
#include "modLwipSafe.h"
#include "mc.defines.h"

#if ESP32
#if ESP32 != 3
	#include "esp32/rom/ets_sys.h"
#endif
	#include "nvs_flash/include/nvs_flash.h"
	#include "esp_partition.h"
	#include "esp_wifi.h"
#else
	#include "tinyprintf.h"
	#include "spi_flash.h"
#endif

#include "xs.h"
#include "xsHosts.h"

#ifdef mxDebug
	#include "modPreference.h"
#endif

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void *)0x87654321)

static void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf);
static void doRemoteCommand(txMachine *the, uint8_t *cmd, uint32_t cmdLen);

#if defined (mxDebug) && ESP32
	SemaphoreHandle_t gDebugMutex;

	#define mxDebugMutexTake() xSemaphoreTake(gDebugMutex, portMAX_DELAY)
	#define mxDebugMutexGive() xSemaphoreGive(gDebugMutex)
	#define mxDebugMutexAllocated() (NULL != gDebugMutex)

	static int fx_vprintf(const char *str, va_list list);
#else
	#define mxDebugMutexTake()
	#define mxDebugMutexGive()
	#define mxDebugMutexAllocated() (true)
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
#ifdef mxDebug
	while (the->debugFragments) {
		DebugFragment next = the->debugFragments->next;
		c_free(the->debugFragments);
		the->debugFragments = next;
	}
#endif

#ifdef mxInstrument
	modInstrumentMachineEnd(the);
#endif

	modMachineTaskUninit(the);
}

uint8_t fxInNetworkDebugLoop(txMachine *the)
{
#ifdef mxDebug
	return the->DEBUG_LOOP && the->connection && (kSerialConnection != the->connection);
#else
	return 0;
#endif
}

void fx_putc(void *refcon, char c)
{
#ifdef mxDebug
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
				mxDebugMutexGive();
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
			mxDebugMutexTake();

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
#endif

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

const char *gXSAbortStrings[] ICACHE_FLASH_ATTR = {
	"debugger",
	"memory full",
	"stack overflow",
	"fatal",
	"dead strip",
	"unhandled exception",
	"not enough keys",
	"too much computation",
	"unhandled rejection"
};

void fxAbort(txMachine* the, int status)
{
#if MODDEF_XS_TEST
	if (XS_DEBUGGER_EXIT == status) {
		extern txMachine *gThe;
		if (gThe == the) {
			gThe = NULL;		// soft reset
			return;
		}
	}
#endif

#if defined(mxDebug) || defined(mxInstrument)
	const char *msg = (status <= XS_UNHANDLED_REJECTION_EXIT) ? gXSAbortStrings[status] : "unknown";

	fxReport(the, "XS abort: %s\n", msg);
	#if defined(mxDebug) && !MODDEF_XS_TEST
		if ((char *)&the <= the->stackLimit)
			the->stackLimit = NULL;
		fxDebugger(the, (char *)__FILE__, __LINE__);
	#endif
#endif

#ifdef MODDEF_XS_RESTARTON
	static const int restart[] = {
		#if defined(mxDebug)
			XS_DEBUGGER_EXIT,
			XS_FATAL_CHECK_EXIT,
		#endif
			MODDEF_XS_RESTARTON };
	int i;
	for (i = 0; i < sizeof(restart) / sizeof(int); i++) {
		if (restart[i] == status)
			c_exit(status);
	}
#else
	#if ESP32
		c_exit(status);
	#else
		system_restart();
		esp_yield();
	#endif
#endif
}

#ifdef mxDebug

static void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength);

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
	err_t result = ERR_OK;

    xmodLog("  didReceive - ENTER");

	if (NULL == p) {
		xmodLog("  didReceive - CONNECTION LOST");
		for (i = 0; i < kDebugReaderCount; i++) {
			if (the->readers[i])
				pbuf_free(the->readers[i]);
			the->readers[i] = NULL;
		}

		if (the->connection) {
/*
			tcp_recv(the->connection, NULL);
			tcp_err(the->connection, NULL);
#if ESP32
#else
			tcp_close(the->connection);
			tcp_abort(the->connection);		// not _safe inside callback. must call tcp_abort on ESP8266 or memory leak
#endif
*/
			the->connection = NULL;

			return ERR_ABRT;
		}
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
			modMessagePostToMachine(the, NULL, 0xffff, doDebugCommand, the);
		}
	}
	else {
		modLog("  debug receive overflow");
		result = ERR_MEM;
	}

	mxDebugMutexGive();

	xmodLog("  didReceive - EXIT");

	return result;
}

static void didError(void *arg, err_t err)
{
	txMachine* the = arg;
	the->connection = NULL;		// "pcb is already freed when this callback is called"
}

static void didErrorConnect(void *arg, err_t err)
{
	txMachine* the = arg;

	the->connection = (void *)-1;
}

static void initializeConnection(txMachine *the)
{
	the->connection = NULL;
	c_memset(the->readers, 0, sizeof(the->readers));
	the->readerOffset = 0;
	the->inPrintf =
	the->debugNotifyOutstanding =
	the->DEBUG_LOOP = 0;
	the->wsState = 0;
	the->wsSendStart = 1;
	while (the->debugFragments) {
		void *tmp = the->debugFragments;
		the->debugFragments = the->debugFragments->next;
		c_free(tmp);
	}
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
	int count;

	initializeConnection(the);

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

		goto connected;
	}

/*
	if (0 == gXSBUG[0]) {
		modLog("  fxConnect - NO XSBUG ADDRESS");
		return;
	}

	modLog("  fxConnect - ENTER");

	pcb = tcp_new();
	if (!pcb) return;

	err = tcp_bind(pcb, IP_ADDR_ANY, 0);
	if (err) {
		tcp_close_safe(pcb);
		return;
	}

#if ESP32
	IP4_ADDR(ip_2_ip4(&ipaddr), gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#else
	IP4_ADDR(&ipaddr, gXSBUG[0], gXSBUG[1], gXSBUG[2], gXSBUG[3]);
#endif

	tcp_arg(pcb, the);
	tcp_recv(pcb, didReceive);
	tcp_err(pcb, didErrorConnect);
//	tcp_nagle_disable(pcb);

    modLog("  fxConnect - connect to XSBUG");
	err = tcp_connect(pcb, &ipaddr, 5002, didConnect);
	if (err) {
		tcp_close_safe(pcb);
		modLog("  fxConnect - tcp_connect ERROR");
		return;
	}

	count = 0;
	while (!the->connection && (count++ < 500))		// 5 seconds, then surrender
		modDelayMilliseconds(10);

	if (!the->connection || ((txSocket)-1 == the->connection)) {
		modLog("  fxConnect - couldn't connect");
		if (NULL == the->connection) {		// timeout.
			tcp_err(pcb, NULL);
			tcp_close_safe(pcb);
		}
		the->connection = NULL;
		return;
	}

	tcp_err(pcb, didError);
*/
connected:
    xmodLog("  fxConnect - EXIT");
	return;
}

void fxDisconnect(txMachine* the)
{
	xmodLog("  fxDisconnect - ENTER");
	if (the->connection) {

		if (kSerialConnection != the->connection) {
/*			uint8_t i, closeMsg[2];
			for (i = 0; i < kDebugReaderCount; i++) {
				if (the->readers[i])
					pbuf_free(the->readers[i]);
				the->readers[i] = NULL;
			}
			closeMsg[0] = 0x88;
			closeMsg[1] = 0;
			tcp_write_safe(the->connection, closeMsg, sizeof(closeMsg), TCP_WRITE_FLAG_COPY);
			tcp_output_safe(the->connection);

			the->readerOffset = 0;
			tcp_recv(the->connection, NULL);
			tcp_err(the->connection, NULL);
			if (18 != the->wsState)
				tcp_close_safe((struct tcp_pcb *)the->connection);
*/		}
		else {
			mxDebugMutexTake();
			fx_putpi(the, '-', true);
			the->debugConnectionVerified = 0;
			mxDebugMutexGive();
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
/*
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
*/
}

void fxReceive(txMachine* the)
{
	struct tcp_pcb *pcb = the->connection;
	if (!pcb) return;

	xmodLog("  fxReceive - ENTER");

	if (kSerialConnection != pcb) {
/*		struct pbuf *p;
		uint16_t use;

		modWatchDogReset();
		if (NULL == the->readers[0]) {
			modDelayMilliseconds(10);
			return;
		}

		p = the->readers[0];
		if (p->next)
			modLog("  fxReceive - cannot handle fragmented!!");

		mxDebugMutexTake();
			use = p->len - the->readerOffset;
			if (use > sizeof(the->debugBuffer))
				use = sizeof(the->debugBuffer);
#if 0
			c_memmove(the->debugBuffer, the->readerOffset + (char *)p->payload, use);
			the->readerOffset += use;
			the->debugOffset = use;
#else
			uint16_t i;
			uint8_t *debugBuffer = the->debugBuffer;
			for (i = 0; i < use; i++) {
				uint8_t byte = *(uint8_t *)(i + the->readerOffset + (char *)p->payload);
				switch (the->wsState) {
					case 0:
						the->wsState = 1;
						the->wsFin = 0 != (0x80 & byte);
						if (NULL == the->wsCmd) {
							if (8 == (byte & 0x0f)) {		// close
								fxDisconnect(the);
								use = 0;
								break;
							}
							the->wsCmd = (2 == (byte & 0x0f)) ? (void *)-1 : NULL;		// binary data is cmd; text is xsbug
						}
						break;
					case 1:
						byte &= 0x7F;
						if (126 == byte)
							the->wsState = 3;
						else {
							the->wsLength = byte;
							the->wsState = 5;
						}
						break;
					case 2:		//@@ unused
						break;
					case 3:
						the->wsLength = byte << 8;
						the->wsState = 4;
						break;
					case 4:
						the->wsLength |= byte;
						the->wsState = 5;
						break;
					case 5:
					case 6:
					case 7:
					case 8:
						the->wsMask[the->wsState - 5] = byte;
						the->wsState += 1;
						if ((9 == the->wsState) && the->wsCmd) {
							if (((void *)-1) == the->wsCmd) {	// new frame
								the->wsCmd = c_malloc(the->wsLength + sizeof(uint32_t));
								*(uint32_t *)the->wsCmd = the->wsLength + sizeof(uint32_t);
								the->wsCmdPtr = the->wsCmd + sizeof(uint32_t);
							}
							else {		// continuation frame
								uint32_t length = *(uint32_t *)the->wsCmd;
								the->wsCmd = c_realloc(the->wsCmd, length + the->wsLength);
								if (the->wsCmd) {
									*(uint32_t *)the->wsCmd = length + the->wsLength;
									the->wsCmdPtr = the->wsCmd + length;
								}
							}
							the->wsState = the->wsCmd ? 13 : 17;
						}
						break;
					case 9:
					case 10:
					case 11:
					case 12:
						*debugBuffer++ = byte ^ the->wsMask[the->wsState - 9];
						the->wsState += 1;
						if (13 == the->wsState)
							the->wsState = 9;
						the->wsLength -= 1;
						if (0 == the->wsLength)
							the->wsState = 0;
						break;

					case 13:
					case 14:
					case 15:
					case 16:
						*(the->wsCmdPtr++) = byte ^ the->wsMask[the->wsState - 13];
						the->wsState += 1;
						if (17 == the->wsState)
							the->wsState = 13;
						the->wsLength -= 1;
						if (0 == the->wsLength) {
							if (the->wsFin) {
								// received full remote command
								doRemoteCommand(the, the->wsCmd + sizeof(uint32_t), the->wsCmdPtr - (the->wsCmd + sizeof(uint32_t)));
								c_free(the->wsCmd);
								the->wsCmd = the->wsCmdPtr = NULL;
							}
							the->wsState = 0;
						}
						break;

					case 17:
						the->wsLength -= 1;
						if (0 == the->wsLength)
							the->wsState = 0;
						break;
					case 18:
						break;		// reserved for disconnecting on restart (don't close socket)

				}
			}
			the->debugOffset = debugBuffer - (uint8_t *)the->debugBuffer;

			the->readerOffset += use;
#endif

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
*/	}
	else {
		uint32_t start = the->debugConnectionVerified ? 0 : modMilliseconds();

		while (!the->debugOffset) {
			if (!the->debugConnectionVerified && (((int)(modMilliseconds() - start)) >= 2000)) {
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

				if (!f->binary) {
					c_memcpy(the->debugBuffer, f->bytes, f->count);
					the->debugOffset = f->count;
				}
				else
					doRemoteCommand(the, f->bytes, f->count);
				c_free(f);
				break;
			}
		}
		if (the->debugOffset)
			the->debugConnectionVerified = 1;
	}
}

void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	txMachine* the = machine;

	the->debugNotifyOutstanding = false;
	if (NULL == the->connection)
		return;
	else if (kSerialConnection == the->connection) {
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
	static uint16_t binary = 0;
	static DebugFragment fragment = NULL;
	static uint32_t value = 0;
	static uint8_t bufferedBytes = 0;
	static uint8_t buffered[28];		//@@ this must be smaller than sxMachine / debugBuffer

	if (!mxDebugMutexAllocated())
		return;

	mxDebugMutexTake();

	modWatchDogReset();

	while (true) {
		int c = ESP_getc();
		if (-1 == c)
			break;

		if ((state >= 0) && (state <= 6)) {
			if (0 == state) {
				current = NULL;
				value = 0;
				binary = 0;
			}
			if (c == c_read8(piBegin + state))
				state++;
			else
			if ((6 == state) && ('#' == c)) {
				binary = 1;
				state++;
			}
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
				if (binary)
					state = 20;
				else {
					state++;
					bufferedBytes = 0;
				}
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
				fragment = c_malloc(sizeof(DebugFragmentRecord) + bufferedBytes);
				if (NULL == fragment) {
					modLog("no fragment memory");
					break;
				}
				fragment->next = NULL;
				fragment->count = bufferedBytes;
				fragment->binary = 0;
				c_memcpy(fragment->bytes, buffered, bufferedBytes);
	enqueue:
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
					modMessagePostToMachine(current, NULL, 0xffff, doDebugCommand, current);
				}
				bufferedBytes = 0;
			}
		}
		else if (20 == state) {
			binary = c << 8;
			state = 21;
		}
		else if (21 == state) {
			binary += c;
			state = 22;

			fragment = c_malloc(sizeof(DebugFragmentRecord) + binary);
			if (NULL == fragment) {
				state = 0;
				continue;
			}
			fragment->next = NULL;
			fragment->count = binary;
			fragment->binary = 1;
			binary = 0;
		}
		else if (22 == state) {
			fragment->bytes[binary++] = c;
			if (fragment->count == binary) {
				state = 0;
				goto enqueue;
			}
		}
	}
	mxDebugMutexGive();
}

void fxSend(txMachine* the, txBoolean flags)
{
	struct tcp_pcb *pcb = the->connection;
	txBoolean more = 0 != (flags & 1);
	txBoolean binary = 0 != (flags & 2);

	xmodLog("  fxSend - ENTER");

	if (!pcb) return;

	if (kSerialConnection != pcb) {
/*		int length = the->echoOffset;
		const char *bytes = the->echoBuffer;
		err_t err;
		uint8_t sentHeader = 0;

		xmodLog("  fxSend - about to loop");

		while (length && the->connection) {
			u16_t available = tcp_sndbuf(pcb);
			if ((0 == available) || (!sentHeader && (available < 4))) {
				xmodLog("  fxSend - need to wait");
				tcp_output_safe(pcb);
				modWatchDogReset();
				modDelayMilliseconds(10);
				continue;
			}

			if (!sentHeader) {
				uint8_t header[4];
				header[0] = (the->wsSendStart ? (binary ? 0x02 : 0x01) : 0) | (more ? 0 : 0x80);
				the->wsSendStart = !more;
				if (length < 126) {
					header[1] = (uint8_t)length;
					sentHeader = 2;
				}
				else {
					header[1] = 126;
					header[2] = length >> 8;
					header[3] = length & 0xff;
					sentHeader = 4;
				}
				tcp_write_safe(pcb, header, sentHeader, more ? (TCP_WRITE_FLAG_MORE | TCP_WRITE_FLAG_COPY) : TCP_WRITE_FLAG_COPY);
				available -= sentHeader;
				if (0 == available)
					continue;
			}

			if (available > length)
				available = length;

			while (the->connection) {
				modWatchDogReset();
				err = tcp_write_safe(pcb, bytes, available, more ? (TCP_WRITE_FLAG_MORE | TCP_WRITE_FLAG_COPY) : TCP_WRITE_FLAG_COPY);
				if (ERR_MEM == err) {
					xmodLog("  fxSend - wait for send memory:");
					tcp_output_safe(pcb);
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
		if (!more && the->connection)
			tcp_output_safe(pcb);
*/	}
	else {
		char *c;
		txInteger count;

		if (!the->inPrintf) {
			mxDebugMutexTake();
			if (binary) {
				fx_putpi(the, '#', true);
				ESP_putc((uint8_t)(the->echoOffset >> 8));
				ESP_putc((uint8_t)the->echoOffset);
			}
			else
				fx_putpi(the, '.', false);
		}
		the->inPrintf = more;

		ESP_put(the->echoBuffer, the->echoOffset);

		if (!more)
			mxDebugMutexGive();
	}

	xmodLog("  fxSend - EXIT");
}

void fxConnectTo(txMachine *the, struct tcp_pcb *pcb)
{
	initializeConnection(the);
/*
	tcp_arg(pcb, the);
	tcp_recv(pcb, didReceive);
	tcp_err(pcb, didError);
*/
	the->connection = pcb;
}

void doRemoteCommand(txMachine *the, uint8_t *cmd, uint32_t cmdLen)
{
	uint16_t resultID = 0;
	int16_t resultCode = 0;
	uint8_t cmdID;
	int baud = 0;

	if (!cmdLen)
		return;

	cmdID = *cmd++;
	cmdLen -= 1;

	if (cmdLen >= 2) {
		resultID = (cmd[0] << 8) | cmd[1];
		cmd += 2, cmdLen -= 2;

		the->echoBuffer[0] = 5;
		the->echoBuffer[1] = resultID >> 8;
		the->echoBuffer[2] = resultID & 0xff;
		the->echoBuffer[3] = 0;
		the->echoBuffer[4] = 0;
		the->echoOffset = 5;
	}

	switch (cmdID) {
		case 1:		// restart
			the->wsState = 18;
			break;

#if MODDEF_XS_MODS
		case 2: {		// uninstall
			uint8_t erase[16] = {0};
#if ESP32
			const esp_partition_t *partition = esp_partition_find_first(0x40, 1,  NULL);
			if (!partition || (ESP_OK != esp_partition_write(partition, 0, erase, sizeof(erase))))
				resultCode = -2;
#else
			uint32_t offset = (uintptr_t)kModulesStart - (uintptr_t)kFlashStart;
			if (!modSPIWrite(offset, sizeof(erase), erase))
				resultCode = -2;
#endif
			} break;

		case 3: {	// install some
			uint32_t offset = c_read32be(cmd);
			cmd += 4, cmdLen -= 4;
#if ESP32
			const esp_partition_t *partition = esp_partition_find_first(0x40, 1,  NULL);
			resultCode = -3;
			if (partition) {
				int firstSector = offset / SPI_FLASH_SEC_SIZE, lastSector = (offset + cmdLen) / SPI_FLASH_SEC_SIZE;
				if (!(offset % SPI_FLASH_SEC_SIZE))			// starts on sector boundary
					esp_partition_erase_range(partition, offset, SPI_FLASH_SEC_SIZE * ((lastSector - firstSector) + 1));
				else if (firstSector != lastSector)
					esp_partition_erase_range(partition, (firstSector + 1) * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE * (lastSector - firstSector));	// crosses into a new sector

				if (ESP_OK == esp_partition_write(partition, offset, cmd, cmdLen))
					resultCode = 0;
			}
#else
			if ((offset + cmdLen) > (kModulesEnd - kModulesStart)) {
				resultCode = -3;
				break;
			}

			offset += (uintptr_t)kModulesStart - (uintptr_t)kFlashStart;

			int firstSector = offset / SPI_FLASH_SEC_SIZE, lastSector = (offset + cmdLen) / SPI_FLASH_SEC_SIZE;
			if (!(offset % SPI_FLASH_SEC_SIZE))			// starts on sector boundary
				modSPIErase(offset, SPI_FLASH_SEC_SIZE * ((lastSector - firstSector) + 1));
			else if (firstSector != lastSector)
				modSPIErase((firstSector + 1) * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE * (lastSector - firstSector));	// crosses into a new sector

			if (!modSPIWrite(offset, cmdLen, cmd))
				resultCode = -3;
#endif
			}
			break;
#endif /* MODDEF_XS_MODS */

		case 4: {	// set preference
			uint8_t *domain = cmd, *key = NULL, *value = NULL;
			while (cmdLen--) {
				if (!*cmd++) {
					if (NULL == key)
						key = cmd;
					else if (NULL == value) {
						value = cmd;
						break;
					}
				}
			}
			if (key && value) {
				uint8_t prefType = c_read8(value++);
				cmdLen -= 1;

				if (!modPreferenceSet(domain, key, prefType, value, cmdLen))
					resultCode = -5;
			}
			}
			break;

		case 6: {		// get preference
			uint8_t *domain = cmd, *key = NULL, *value = NULL;
			int zeros = 0;
			while (cmdLen--) {
				if (!*cmd++) {
					zeros += 1;
					if (NULL == key)
						key = cmd;
					else
						break;
				}
			}
			if ((2 == zeros) && key) {
				if (NULL != c_strstr(key, "password")) {
					resultCode = -4;
					break;
				}

				uint8_t *buffer = the->echoBuffer + the->echoOffset;
				uint16_t byteCountOut;
				if (!modPreferenceGet(domain, key, buffer, buffer + 1, sizeof(the->echoBuffer) - (the->echoOffset + 1), &byteCountOut))
					resultCode = -6;
				else
					the->echoOffset += byteCountOut + 1;
			}
		}
		break;

		case 8:		// set baud
			baud = c_read32be(cmd);
			break;

		case 9:
#if !MODDEF_XS_DONTINITIALIZETIME
			if (cmdLen >= 4)
				modSetTime(c_read32be(cmd + 0));
			if (cmdLen >= 8)
				modSetTimeZone(c_read32be(cmd + 4));
			if (cmdLen >= 12)
				modSetDaylightSavingsOffset(c_read32be(cmd + 8));
#endif
			break;

		case 11:
			the->echoBuffer[the->echoOffset++] = XS_MAJOR_VERSION;
			the->echoBuffer[the->echoOffset++] = XS_MINOR_VERSION;
			the->echoBuffer[the->echoOffset++] = XS_PATCH_VERSION;
			break;

		case 12:
			the->echoBuffer[the->echoOffset++] = kCommodettoBitmapFormat;
			the->echoBuffer[the->echoOffset++] = kPocoRotation / 90;
			break;

		case 13:
			c_strcpy(the->echoBuffer + the->echoOffset, PIU_DOT_SIGNATURE);
			the->echoOffset += c_strlen(the->echoBuffer + the->echoOffset);
			break;

		case 14:
#if ESP32
			esp_efuse_mac_get_default(the->echoBuffer + the->echoOffset);
#else
			wifi_get_macaddr(0 /* STATION_IF */, the->echoBuffer + the->echoOffset);
#endif
			the->echoOffset += 6;
			break;

#if MODDEF_XS_MODS
		case 15: {
#if ESP32
			uint32_t freeRAM = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_8BIT);
#else
			uint32_t freeRAM = (uint32_t)system_get_free_heap_size();
#endif
			uint32_t installSpace = kModulesByteLength;
			uint32_t transferSize;

			the->echoBuffer[the->echoOffset++] = installSpace >> 24;
			the->echoBuffer[the->echoOffset++] = installSpace >> 16;
			the->echoBuffer[the->echoOffset++] = installSpace >>  8;
			the->echoBuffer[the->echoOffset++] = installSpace;

			transferSize = freeRAM >> 2;
			if (transferSize > 4096)
				transferSize = 4096;

			the->echoBuffer[the->echoOffset++] = transferSize >> 24;
			the->echoBuffer[the->echoOffset++] = transferSize >> 16;
			the->echoBuffer[the->echoOffset++] = transferSize >>  8;
			the->echoBuffer[the->echoOffset++] = transferSize;
			} break;

		case 16: {
			int atomSize;
			char *atom = modGetModAtom(the, c_read32be(cmd), &atomSize);
			if (atom && (atomSize <= (sizeof(the->echoBuffer) - the->echoOffset))) {
				c_memcpy(the->echoBuffer + the->echoOffset, atom, atomSize);
				the->echoOffset += atomSize;
			}
			} break;
#endif

		case 17:
			the->echoBuffer[the->echoOffset++] = 'e';
			the->echoBuffer[the->echoOffset++] = 's';
			the->echoBuffer[the->echoOffset++] = 'p';
#if ESP32
			the->echoBuffer[the->echoOffset++] = '3';
			the->echoBuffer[the->echoOffset++] = '2';
#if kCPUESP32S2
			the->echoBuffer[the->echoOffset++] = '-';
			the->echoBuffer[the->echoOffset++] = 's';
			the->echoBuffer[the->echoOffset++] = '2';
#elif kCPUESP32S3
			the->echoBuffer[the->echoOffset++] = '-';
			the->echoBuffer[the->echoOffset++] = 's';
			the->echoBuffer[the->echoOffset++] = '3';
#endif

#endif
			break;

		default:
#if MODDEF_XS_MODS
			resultCode = -7;
#else
			resultCode = -8;
#endif
			break;
	}

bail:
	if (resultID) {
		the->echoBuffer[3] = resultCode >> 8;
		the->echoBuffer[4] = resultCode & 0xff;
		fxSend(the, 2);		// send binary
	}

	// finish command after sending reply
	switch (cmdID) {
		case 1:		// restart
			fxDisconnect(the);
			modDelayMilliseconds(1000);
#if ESP32
			esp_restart();
#else
			system_restart();
#endif
			while (1)
				modDelayMilliseconds(1000);
			break;

		case 8:		// set baud
			if (baud)
				ESP_setBaud(baud);
			break;
	}
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

