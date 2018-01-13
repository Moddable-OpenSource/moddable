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

#include "xsgecko.h"

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void*)0x87654321)

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
            static const char *xsbugHeader = "\r\n<?xs.87654321?>\r\n<xsbug><log>";
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
#endif


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
		the->connection = (txSocket)kSerialConnection;
		return;
	}

}

void fxDisconnect(txMachine* the)
{
	if (the->connection) {
		ESP_putc('\r');
		ESP_putc('\n');
		ESP_putc('<');
		ESP_putc('?');
		ESP_putc('x');
		ESP_putc('s');
		ESP_putc('-');
		ESP_putc('8');
		ESP_putc('7');
		ESP_putc('6');
		ESP_putc('5');
		ESP_putc('4');
		ESP_putc('3');
		ESP_putc('2');
		ESP_putc('1');
		ESP_putc('?');
		ESP_putc('>');
		ESP_putc('\r');
		ESP_putc('\n');
	}
	the->connection = (txSocket)NULL;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if ((txSocket)kSerialConnection == the->connection)
		return (txBoolean)ESP_isReadable();

	return 0;
}

void fxReceive(txMachine* the)
{
	the->debugOffset = 0;

	if ((txSocket)kSerialConnection == the->connection) {
		while (the->debugOffset < (sizeof(the->debugBuffer) - 3)) {
			int c = ESP_getc();
			if (-1 == c) {
//				modDelayMilliseconds(2);
				c = ESP_getc();
				if (-1 == c)
					break;
			}

			the->debugBuffer[the->debugOffset++] = (txU1)c;

		}
	}

	the->debugBuffer[the->debugOffset] = 0;
}

void fxSend(txMachine* the, txBoolean more)
{
	if ((txSocket)kSerialConnection == the->connection) {
		char *c = the->echoBuffer;
		txInteger count = the->echoOffset;
		if (!the->inPrintf) {
			ESP_putc('\r');
			ESP_putc('\n');
			ESP_putc('<');
			ESP_putc('?');
			ESP_putc('x');
			ESP_putc('s');
			ESP_putc('.');
			ESP_putc('8');
			ESP_putc('7');
			ESP_putc('6');
			ESP_putc('5');
			ESP_putc('4');
			ESP_putc('3');
			ESP_putc('2');
			ESP_putc('1');
			ESP_putc('?');
			ESP_putc('>');
		}
		the->inPrintf = more;
		while (count--)
			ESP_putc(*c++);
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



