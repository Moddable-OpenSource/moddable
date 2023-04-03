/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "xsPlatform.h"

#include "mc.defines.h"
#include "xs.h"
#include "xsHosts.h"
#include "xsHost.h"
#include "pico/sem.h"

#ifdef mxDebug
	#include "modPreference.h"
#endif

#define XSDEBUG_NONE	0,0,0,0
#define XSDEBUG_SERIAL	127,0,0,7

#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

uint8_t gXSBUG[4] = { DEBUG_IP };


#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((txSocket)0x87654321)

static void fx_putpi(txMachine *the, char separator, txBoolean trailingcrlf);
static void doRemoteCommand(txMachine *the, uint8_t *cmd, uint32_t cmdLen);
void fxReceiveLoop(void);

static semaphore_t gDebugMutex;
static int gDebugMutexInited = 0;
#define mxDebugMutexTake()	sem_acquire_blocking(&gDebugMutex)
#define mxDebugMutexGive()	sem_release(&gDebugMutex)
#define mxDebugMutexAllocated() (gDebugMutexInited)


void fxCreateMachinePlatform(txMachine* the)
{
	modMachineTaskInit(the);
#ifdef mxDebug
	the->connection = (txSocket)mxNoSocket;
	if (!gDebugMutexInited) {
		sem_init(&gDebugMutex, 1, 1);
		gDebugMutexInited = 1;
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

#ifdef mxInstrument
	modInstrumentMachineEnd(the);
#endif

	modMachineTaskUninit(the);
}

void fx_putc(void *refcon, char c)
{
#ifdef mxDebug
    txMachine* the = refcon;

    if (the->inPrintf) {
        if (0 == c) {
            if (kSerialConnection == the->connection) {
                // write xsbug log trailer
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
		fxDebugger(the, (char*)__FILE__, __LINE__);
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
	c_exit(status);
#endif
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
	}
}

void fxDisconnect(txMachine* the)
{
	if (the->connection) {
		mxDebugMutexTake();
		fx_putpi(the, '-', true);
		the->debugConnectionVerified = 0;
		mxDebugMutexGive();
		//@@ clear debug fragments?
	}
	the->connection = (txSocket)NULL;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (kSerialConnection == the->connection) {
		fxReceiveLoop();
//		taskYIELD();
		return NULL != the->debugFragments;
	}

	return 0;
}

#define kXsbugConnectionTimeout 4000	//@@ was 2000 - needs revisiting (pico hosted from ubuntu VM)
void fxReceive(txMachine* the)
{
	the->debugOffset = 0;

	if (kSerialConnection == the->connection) {
		uint32_t start = the->debugConnectionVerified ? 0 : modMilliseconds();

		while (!the->debugOffset) {
			if (!the->debugConnectionVerified && (((int)modMilliseconds() - start) >= kXsbugConnectionTimeout)) {
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
	if ((txSocket)NULL == the->connection)
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
	txBoolean more = 0 != (flags & 1);
	txBoolean binary = 0 != (flags & 2);

	if (kSerialConnection == the->connection) {
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

		c = the->echoBuffer;
		count = the->echoOffset;
		while (count--)
			ESP_putc(*c++);

		if (!more)
			mxDebugMutexGive();
	}
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
//printf("do Remote command %d\n", cmdID);

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
			break;

#if MODDEF_XS_MODS
		case 2: {		// uninstall
			uint8_t erase[16] = {0};
			uint32_t offset = (uintptr_t)kModulesStart - (uintptr_t)kFlashStart;
			if (!modSPIWrite(offset, sizeof(erase), erase))
				resultCode = -1;
			} break;

		case 3: {	// install some
			uint32_t offset = c_read32be(cmd);
			cmd += 4, cmdLen -= 4;
			if ((offset + cmdLen) > kModulesByteLength) {
				resultCode = -2;
				break;
			}

			offset += (uintptr_t)kModulesStart - (uintptr_t)kFlashStart;

			int firstSector = offset / kFlashSectorSize, lastSector = (offset + cmdLen) / kFlashSectorSize;
			if (!(offset % kFlashSectorSize))			// starts on sector boundary
				modSPIErase(offset, kFlashSectorSize * ((lastSector - firstSector) + 1));
			else if (firstSector != lastSector)
				modSPIErase((firstSector + 1) * kFlashSectorSize, kFlashSectorSize * (lastSector - firstSector));	// crosses into a new sector

			if (!modSPIWrite(offset, cmdLen, cmd))
				resultCode = -1;
			}
			break;
#endif /* MODDEF_XS_MODS */

		case 4: {	// set preference
			uint8_t *domain = cmd, *key = NULL, *value = NULL;
printf("set pref\n");
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
					resultCode = -1;
			}
			} break;

		case 6: {		// get preference
			uint8_t *domain = cmd, *key = NULL, *value = NULL;
printf("get pref\n");
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
				uint8_t buffer[65];
				uint8_t type;
				uint16_t byteCountOut;
				if (!modPreferenceGet(domain, key, &buffer[0], buffer + 1, sizeof(buffer), &byteCountOut))
					resultCode = -1;
				else {
					c_memcpy(the->echoBuffer + the->echoOffset, buffer, byteCountOut + 1);
					the->echoOffset += byteCountOut + 1;
				}
			}
		} break;

		case 9:
			if (cmdLen >= 4)
				modSetTime(c_read32be(cmd + 0));
			if (cmdLen >= 8)
				modSetTimeZone(c_read32be(cmd + 4));
			if (cmdLen >= 12)
				modSetDaylightSavingsOffset(c_read32be(cmd + 8));
			break;

		case 11:
printf("get vers\n");
			the->echoBuffer[the->echoOffset++] = XS_MAJOR_VERSION;
			the->echoBuffer[the->echoOffset++] = XS_MINOR_VERSION;
			the->echoBuffer[the->echoOffset++] = XS_PATCH_VERSION;
			break;

		case 12:
printf("get format\n");
			the->echoBuffer[the->echoOffset++] = kCommodettoBitmapFormat;
			the->echoBuffer[the->echoOffset++] = kPocoRotation / 90;
			break;

		case 13:
printf("get sig\n");
			c_strcpy(the->echoBuffer + the->echoOffset, PIU_DOT_SIGNATURE);
			the->echoOffset += c_strlen(the->echoBuffer + the->echoOffset);
			break;
		
		case 14:  {
printf("get id\n");
			uint8_t *id = (uint8_t *)(the->echoBuffer + the->echoOffset);
			pico_get_mac(id);
			the->echoOffset += 8;
			} break;

#if MODDEF_XS_MODS
		case 15:
			the->echoBuffer[the->echoOffset++] = kModulesByteLength >> 24;
			the->echoBuffer[the->echoOffset++] = kModulesByteLength >> 16;
			the->echoBuffer[the->echoOffset++] = kModulesByteLength >>  8;
			the->echoBuffer[the->echoOffset++] = kModulesByteLength;
			break;

		case 16: {
			int atomSize;
			char *atom = modGetModAtom(the, c_read32be(cmd), &atomSize);
			if (atom && (atomSize <= (sizeof(the->echoBuffer) - the->echoOffset))) {
				c_memcpy(the->echoBuffer + the->echoOffset, atom, atomSize);
				the->echoOffset += atomSize;
			}
			} break;
#endif

		default:
printf("unknown cmdID: %d\n");
			resultCode = -3;
			break;
	}

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

			pico_reset();

			while (1)
				modDelayMilliseconds(1000);
			break;
	}

}

#endif /* mxDebug */

uint8_t fxInNetworkDebugLoop(txMachine *the)
{
#ifdef mxDebug
	return the->DEBUG_LOOP && the->connection && (kSerialConnection != the->connection);
#else
	return 0;
#endif
}

uint32_t pico_memory_remaining() {
	return (1024);
}


