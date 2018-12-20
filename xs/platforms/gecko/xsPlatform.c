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
#include <stdio.h>

#include "xsgecko.h"

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void*)0x87654321)

static void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf);
void fxReceiveLoop(void);

#define mxDebugMutexTake()
#define mxDebugMutexGive()

int modMessagePostToMachine(txMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
int modMessagePostToMachineFromPool(txMachine *the, modMessageDeliver callback, void *refcon);
int modMessageService(void);

void modMachineTaskInit(txMachine *the);
void modMachineTaskUninit(txMachine *the);
void modMachineTaskWait(txMachine *the);
void modMachineTaskWake(txMachine *the);

void fxCreateMachinePlatform(txMachine* the)
{
	modMachineTaskInit(the);
#ifdef mxDebug
	the->connection = (txSocket)mxNoSocket;
	the->debugOnReceive = true;
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

#if 0
void fx_putc(void *unused, char c) {
	ESP_putc(c);
}
#else
void fx_putc(void *refcon, char c)
{
	txMachine* the = refcon;

	if (the->inPrintf) {
		if (0 == c) {
			if ((txSocket)kSerialConnection == the->connection) {
				// write xsbug log trailer
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
		if ((txSocket)kSerialConnection == the->connection) {
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
#endif

void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf)
{
	static const char *xsbugHeaderStart = "\r\n<?xs";
	static const char *xsbugHeaderEnd = "?>";
	static const char *gHex = "0123456789ABCDEF";
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

uint8_t triggerDebugCommand(txMachine *the)
{
	if (the->debugOnReceive) {
		fxDebugCommand(the);
		return the->breakOnStartFlag;
	}

	return false;
}


void fxConnect(txMachine* the)
{
	extern unsigned char gXSBUG[4];

	if (isSerialIP(gXSBUG)) {
		static txBoolean once = false;

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

		the->connection = (txSocket)kSerialConnection;

		return;
	}

}

void fxDisconnect(txMachine* the)
{
	if (the->connection) {
		fx_putpi(the, '-', true);
	}
	the->connection = (txSocket)NULL;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if ((txSocket)kSerialConnection == the->connection) {
		fxReceiveLoop();
		return NULL != the->debugFragments;
//		return (txBoolean)ESP_isReadable();
	}

	return 0;
}

void fxReceive(txMachine* the)
{
	the->debugOffset = 0;

	if ((txSocket)kSerialConnection == the->connection) {
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
	if ((txSocket)kSerialConnection == the->connection) {
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



#define XSDEBUG_NONE	0,0,0,0
#define XSDEBUG_SERIAL	127,0,0,7

#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

unsigned char gXSBUG[4] = {DEBUG_IP};



