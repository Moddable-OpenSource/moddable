/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 *
 *     Portions Copyright (C) 2010-2015 Marvell International Ltd.
 *     Portions Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Portions Copyright by Marvell International Ltd. and Kinoma, Inc. are 
 *     derived from KinomaJS/XS6 and used under the Apache 2.0 License.
 */

#include "xsAll.h"
#include "stdio.h"

#define XSDEBUG_NONE	0,0,0,0
#define XSDEBUG_SERIAL	127,0,0,7

#ifndef DEBUG_IP
	#define DEBUG_IP XSDEBUG_SERIAL
#endif

unsigned char gXSBUG[4] = {DEBUG_IP};

#define isSerialIP(ip) ((127 == ip[0]) && (0 == ip[1]) && (0 == ip[2]) && (7 == ip[3]))
#define kSerialConnection ((void*)0x87654321)

void ESP_putc(int c);
int ESP_getc(void);
uint8_t ESP_isReadable();

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
	the->debugOnReceive = true;
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

#if 1
void fx_putc(void *unused, char c) {
	ESP_putc(c);
}
#else
void fx_putc(void *refcon, char c)
{
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
		the->connection = kSerialConnection;
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
	the->connection = NULL;
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connection ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	if (kSerialConnection == the->connection)
		return (txBoolean)ESP_isReadable();

	return 0;
}

void fxReceive(txMachine* the)
{
	the->debugOffset = 0;

	if (kSerialConnection == the->connection) {
		txU1 stop = 0, state = 0;

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

bail:
	the->debugBuffer[the->debugOffset] = 0;

}

void fxSend(txMachine* the, txBoolean more)
{
	if (kSerialConnection == the->connection) {
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


void qsort(
   void *base,
   size_t num,
   size_t width,
   int (*compare )(const void *, const void *)
)
{
	//@@
}

static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time

void modSetTime(uint32_t seconds)
{
	struct timeval tv;
	struct timezone tz;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;

//  c_settimeofday(&tv, NULL);
//          //// NEED TO IMPLEMENT SETTIMEOFDAY
}

void modSetTimeZone(int32_t timeZoneOffset)
{
	gTimeZoneOffset = timeZoneOffset;
}

int32_t modGetTimeZone(void)
{
	return gTimeZoneOffset;
}

void modSetDaylightSavingsOffset(int32_t daylightSavings)
{
	gDaylightSavings = daylightSavings;
}

int32_t modGetDaylightSavingsOffset(void)
{
	return gDaylightSavings;
}

