/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "xsHost.h"

#include "qurt_mutex.h"
#include "qapi_reset.h"

#ifdef mxInstrument
	extern void espDescribeInstrumentation(txMachine *the);
#endif

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void*)0x87654321)

static void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf);
static void doRemoteCommand(txMachine *the, uint8_t *cmd, uint32_t cmdLen);

void fxReceiveLoop(void);

#if defined (mxDebug)
	extern qurt_mutex_t gDebugMutex;
	#define mxDebugMutexTake()  qurt_mutex_lock(&gDebugMutex);
	#define mxDebugMutexGive()  qurt_mutex_unlock(&gDebugMutex);
	#define mxDebugMutexAllocated()  (NULL != (void*)gDebugMutex)
#else
	#define mxDebugMutexTake()
	#define mxDebugMutexGive()
	#define mxDebugMutexAllocated()  (true)
#endif


int modMessagePostToMachine(txMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);	// @@

int modMessageService(void);

void modMachineTaskInit(txMachine *the);
void modMachineTaskUninit(txMachine *the);
void modMachineTaskWake(txMachine *the);


void fxCreateMachinePlatform(txMachine* the)
{
	modMachineTaskInit(the);
#ifdef mxDebug
	the->connection = (txSocket)mxNoSocket;
	if (!gDebugMutex) {
		qurt_mutex_create(&gDebugMutex);
	}
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

	modMachineTaskUninit(the);
}

void fx_putc(void *refcon, char c)
{
#ifdef mxDebug
    txMachine* the = refcon;

    if (the->inPrintf) {
        if (0 == c) {
            if ((txSocket)kSerialConnection == the->connection) {
                // write xsbug log trailer
                const static const char *xsbugTrailer = "&#10;</log></xsbug>\r\n";
				debugger_write(xsbugTrailer, c_strlen(xsbugTrailer));
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
        if ((txSocket)kSerialConnection == the->connection) {
			mxDebugMutexTake();

            // write xsbug log header
            static const char *xsbugHeader = "<xsbug><log>";
			fx_putpi(the, '.', true);
			debugger_write(xsbugHeader, c_strlen(xsbugHeader));
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
    signed char i;

	debugger_write(xsbugHeaderStart, c_strlen(xsbugHeaderStart));

    ESP_putc(separator);

    for (i = 7; i >= 0; i--)
        ESP_putc(c_read8(gHex + ((((uintptr_t)the) >> (i << 2)) & 0x0F)));

	debugger_write(xsbugHeaderEnd, c_strlen(xsbugHeaderEnd));

    if (trailingcrlf) {
		debugger_write("\r\n", 2);
    }
}

void fxAbort(txMachine* the, int status)
{
	c_exit(status);
}

#ifdef mxDebug

static void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength);

void fxConnect(txMachine* the)
{
	extern unsigned char gXSBUG[4];

	if (isSerialIP(gXSBUG)) {
		static txBoolean once = false;

		if (!once) {
			static const char *piReset = "<?xs-00000000?>\r\n";

			modDelayMilliseconds(250);

			debugger_write(piReset, c_strlen(piReset));

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
		the->debugConnectionVerified = 0;
	}
	the->connection = (txSocket)NULL;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	fxReceiveLoop();			// necessary?
	if ((txSocket)kSerialConnection == the->connection) {
		return NULL != the->debugFragments;
	}

	return 0;
}

void fxReceive(txMachine* the)
{
//	the->debugOffset = 0;

	if ((txSocket)kSerialConnection == the->connection) {

		uint32_t timeout = the->debugConnectionVerified ? 0 : (modMilliseconds() + 2000);

		while (!the->debugOffset) {
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
		the->debugConnectionVerified = 1;
	}

	the->debugBuffer[the->debugOffset] = 0;

	xmodLog("  fxReceive - EXIT with:");
	if (the->debugOffset)
		xmodLogVar(the->debugBuffer);
}

void doDebugCommand(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	txMachine* the = machine;

	the->debugNotifyOutstanding = false;

	if (!the->debugFragments)
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
	static uint16_t binary = 0;
	static DebugFragment fragment = NULL;
	static uint32_t value = 0;
	static uint8_t bufferedBytes = 0;
	static uint8_t buffered[28];		//@@ this must be smaller than sxMachine / debugBuffer

	qca4020_watchdog();

	if (!mxDebugMutexAllocated())
		return;

	mxDebugMutexTake();

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
					modMessagePostToMachine(current, NULL, 0, doDebugCommand, current);
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
	txBoolean more = 0 != (flags & 1);
	txBoolean binary = 0 != (flags & 2);

	if ((txSocket)kSerialConnection == the->connection) {
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
		debugger_write(the->echoBuffer, the->echoOffset);

		if (!more)
			mxDebugMutexGive();
	}
}

static void doLoadModule(modTimer timer, void *refcon, int refconSize)
{
	modLoadModule((txMachine *)*(uintptr_t *)refcon, sizeof(uintptr_t) + (uint8_t *)refcon);
}

static void doRemoteCommand(txMachine *the, uint8_t *cmd, uint32_t cmdLen)
{
	uint16_t resultID = 0;
	int16_t resultCode = 0;
	uint8_t	cmdID;
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
			fxDisconnect(the);
			modDelayMilliseconds(1000);
			qca4020_restart();
			while(1)
				modDelayMilliseconds(1000);
			return;

		case 2: 	// uninstall
			break;

		case 3:		// install some
			break;

		case 4:		// set preference
			break;

		case 6:		// get preference
			break;

		case 8:
			baud = c_read32be(cmd);
			break;

		case 9:
			if (cmdLen >= 4)
				modSetTime(c_read32be(cmd + 0));
			if (cmdLen >= 8)
				modSetTimeZone(c_read32be(cmd + 4));
			if (cmdLen >= 12)
				modSetDaylightSavingsOffset(c_read32be(cmd + 8));
			break;

		case 10: {
				uintptr_t bytes[16];
				bytes[0] = (uintptr_t)the;
				if (cmdLen > (sizeof(uintptr_t) * 15)) {
					resultCode = -1;
					goto bail;
				}

				c_strcpy((void *)(bytes + 1), cmd);
				modTimerAdd(0, 0, doLoadModule, bytes, cmdLen + sizeof(uintptr_t));
			}
			break;

		default:
			modLog("unrecognized command");
			modLogInt(cmdID);
			resultCode = -1;
			break;
	}

bail:
	if (resultID) {
		the->echoBuffer[3] = resultCode >> 8;
		the->echoBuffer[4] = resultCode & 0xff;
		fxSend(the, 2);		// send binary
	}

	// if (baud)
	//		teardown serial debug connection and re-establish with new baud
}

#endif /* mxDebug */



#define XSDEBUG_NONE	0,0,0,0
#define XSDEBUG_SERIAL	127,0,0,7

#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

uint8_t gXSBUG[4] = { DEBUG_IP };

//-------------------------------------------------------------------------
#include "qapi_spi_master.h"

static char* qca4020_error_message(int ret) {
	if (!ret)
		return "";
	switch(ret) {
		case QAPI_SPIM_ERROR_INVALID_PARAM:
			return "QAPI_SPIM_ERROR_INVALID_PARAM";
		case QAPI_SPIM_ERROR_CLK_ENABLE_FAIL:
			return "QAPI_SPIM_ERROR_CLK_ENABLE_FAIL";
		case QAPI_SPIM_ERROR_GPIO_ENABLE_FAIL:
			return "QAPI_SPIM_ERROR_GPIO_ENABLE_FAIL";
		case QAPI_SPIM_ERROR_CLK_DISABLE_FAIL:
			return "QAPI_SPIM_ERROR_CLK_DISABLE_FAIL";
		case QAPI_SPIM_ERROR_GPIO_DISABLE_FAIL:
			return "QAPI_SPIM_ERROR_GPIO_DISABLE_FAIL";
		case QAPI_SPIM_ERROR_QUP_STATE_INVALID:
			return "QAPI_SPIM_ERROR_QUP_STATE_INVALID";
		case QAPI_SPIM_ERROR_MEM_ALLOC:
			return "QAPI_SPIM_ERROR_MEM_ALLOC";
		case QAPI_SPIM_ERROR_TRANSFER_TIMEOUT:
			return "QAPI_SPIM_ERROR_TRANSFER_TIMEOUT";
		case QAPI_SPIM_ERR_INTERRUPT_REGISTER:
			return "QAPI_SPIM_ERROR_INTERRUPT_REGISTER";
		default:
			return "other error";
	}
}

static int digit2String(int d, char *s) {
    int sign = 0;
    int i, p = 0;
    char tmp[20];

    if (d < 0) {
        *s++ = '-';
        sign = 1;
        d = -d;
    }
    do {
        i = d % 10;
        d = d / 10;
        tmp[p++] = i + '0';
    } while (d > 0);
    tmp[p] = '\0';
    for (i = p-1; i >= 0; i--)
        *s++ = tmp[i];
    return (p + sign);
}

void qca4020_error(char *bar, int ret) {
	char str[256];
	char *msg;
	int p = 0;

	if (!ret) return;
	sprintf(str, "ERROR: ");
	p = 7;
	strcpy(&str[p], bar);
	p += strlen(bar);
	str[p++] = ' ';
	p += digit2String(ret, &str[p]);
	msg = qca4020_error_message(ret);
	if (msg) {
		str[p++] = ' ';
		strcpy(&str[p], msg);
		p += strlen(msg);
	}
	strcpy(&str[p], "\r\n");
	p += 2;
	debugger_write(str, p);
}

void qca4020_msg_num(char *bar, int num) {
	char str[256];
	int p = 0;

	sprintf(str, "MSG: ");
	p = 5;
	strcpy(&str[p], bar);
	p += strlen(bar);
	str[p++] = ' ';
	p += digit2String(num, &str[p]);
	strcpy(&str[p], "\r\n");
	p += 2;
	debugger_write(str, p);
}

void qca4020_restart(void) {
	qapi_System_Reset();
}
