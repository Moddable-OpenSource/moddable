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
 */

#include "xsmc.h"
#include "mc.xs.h"
#include "modGPIO.h"
#include "modSerial.h"
#include "xsHost.h"

#include "mc.defines.h"


typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	uint16_t	interval;
	uint16_t	timeoutMS;
	uint32_t	timeoutEndMS;
	uint8_t		trace;
	uint8_t		trim;
	uint16_t	bufferSize;
	uint8_t		*buffer;
	char		terminators[MAX_TERMINATORS];

	modSerialConfigRecord config;
	modSerialDevice	device;
} modSerialRecord, *modSerial;


static uint8_t gSerialInited = 0;

#ifndef MODDEF_SERIAL_SAMPLING_INTERVAL
	#define MODDEF_SERIAL_SAMPLING_INTERVAL (100)
#endif

void xs_serial_setBaudrate(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int baud = xsmcToInteger(xsArg(0));
	modSerial_setBaudrate(serial->device, baud);
}

void xs_serial_setTimeout(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int timeoutMS = xsmcToInteger(xsArg(0));
	modSerial_setTimeout(serial->device, timeoutMS);
}

void xs_serial_available(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int avail = modSerial_available(serial->device);
	xsmcSetInteger(xsResult, avail);
}

void xs_serial_availableForWrite(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int avail = ! modSerial_txBusy(serial->device);
	xsmcSetInteger(xsResult, avail);
}

void xs_serial_flush(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	modSerial_flush(serial->device);
}

void xs_serial_peek(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int val = modSerial_peek(serial->device);
	xsmcSetInteger(xsResult, val);
}

void xs_serial_write(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int start = 0, ret, len;
	char *p;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		int bufferLen = xsmcGetArrayBufferLength(xsArg(0));
		if (serial->trace)
			xsTraceRight("write ArrayBuffer", "serial");
		p = xsmcToArrayBuffer(xsArg(0));
		if (xsmcArgc > 1) {
			len = xsmcToInteger(xsArg(1));
			if (len < 0) {
				len = -len;
				start = bufferLen - len;
			}
		}
		else
			len = bufferLen;
	}
	else if (xsmcArgc > 1) {
		p = xsmcToString(xsArg(0));
		len = xsmcToInteger(xsArg(1));
		if (len < 0) {
			len = -len;
			start = c_strlen(p) - len;
		}
		if (serial->trace) {
			xsTraceRight("write String", "xsSerial");
			xsTraceRight(p, "xsSerial");
		}
	}
	else {
		p = xsmcToString(xsArg(0));
		len = c_strlen(p);
		if (serial->trace) {
			xsTraceRight("write String", "xsSerial");
			xsTraceRight(p, "xsSerial");
		}
	}

	if (xsmcArgc > 2) {
		start = len;
		len = xsmcToInteger(xsArg(2));
	}

	if (serial->trace)
		xsTraceRightBytes(p+start, len, "xsSerial");

	ret = modSerial_write(serial->device, (uint8_t*)(p + start), len);
	xsmcSetInteger(xsResult, ret);
}

void xs_serial_read(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int ret;
	uint8_t val;

	ret = modSerial_read(serial->device, &val, 1);
	if (ret)
		xsmcSetInteger(xsResult, val);
	else
		xsmcSetInteger(xsResult, -1);
}

/* readBytes:
	if given a number, it will allocate space for a string and read
	up to that many bytes.
	if given an arrayBuffer, it will try to fill the array buffer. 
		if also given a second parameter length, it will try to
		read length amount of bytes up to the size of the array buffer.
 */
void xs_serial_readBytes(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int len = 0;
	int ret;
	char *str;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		int bufferLen = xsmcGetArrayBufferLength(xsArg(0));
		char *p;

		if (serial->trace)
			xsTraceLeft("read ArrayBuffer", "xsSerial");
		p = xsmcToArrayBuffer(xsArg(0));

		if (xsmcArgc > 1) {
			len = xsmcToInteger(xsArg(1));
			if (len > bufferLen) len = bufferLen;
		}
		else
			len = bufferLen;

		ret = modSerial_read(serial->device, (uint8_t*)p, len);
		if (ret > 0) {
			if (serial->trace) {
				xsTraceLeft("readBytes", "xsSerial");
				xsTraceLeftBytes(p, ret, "xsSerial");
			}
		}
		xsmcSetInteger(xsResult, ret);
		return;
	}

	len = xsmcToInteger(xsArg(0));
	str = (char*)c_malloc(len);
	ret = modSerial_read(serial->device, (uint8_t*)str, len);
	if (ret < len) {
		if (ret)
			str[ret] = '\0';		// null terminate if there is space
	}

	if (ret > 0) {
		if (serial->trace) {
			xsTraceLeft("readBytes", "xsSerial");
			xsTraceLeftBytes(str, ret, "xsSerial");
		}
		xsmcSetString(xsResult, str);
	}

	c_free(str);
}

int serialReadUntil(modSerial serial, char *terminators, char *target, int len) {
	int ret = 0, pos = 0;
	int numComp, i;

	serial->timeoutEndMS = serial->timeoutMS + modMilliseconds();

	numComp = c_strlen(terminators);
	
	while (len) {
		if (modSerial_read(serial->device, (uint8_t*)(target+pos), 1)) {
			for (i=0; i<numComp; i++) {
				if (*(target+pos) == terminators[i]) {
					ret = pos;
					*(target+pos) = '\0';
					goto gotit;
				}
			}
			pos++;
			len--;
		}
		else {
			if (modMilliseconds() > serial->timeoutMS) {
				if (pos<len)
					*(target+pos) = '\0';
				ret = pos;
				break;
			}
		}
	}

gotit:
	return ret;
}

void xs_serial_readBytesUntil(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	char *terminators;
	int len;
	char *str;
	int num;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		int bufferLen = xsmcGetArrayBufferLength(xsArg(0));
		char *p;

		p = xsmcToArrayBuffer(xsArg(0));
		terminators = xsmcToString(xsArg(1));

		if (xsmcArgc > 2) {
			len = xsmcToInteger(xsArg(2));
			if (len > bufferLen) len = bufferLen;
		}
		else
			len = bufferLen;

		num = serialReadUntil(serial, terminators, p, len);

		if (serial->trace) {
			xsTraceLeft("read ArrayBuffer", "xsSerial");
			if (num > 0)
				xsTraceLeftBytes(p, num, "xsSerial");
			if (num < 0)
				xsTraceLeft("error", "xsSerial");
		}
		xsmcSetInteger(xsResult, num);
		return;
	}

	terminators = xsmcToString(xsArg(0));
	len = xsmcToInteger(xsArg(1));

	str = (char*)c_malloc(len+1);
	num = serialReadUntil(serial, terminators, str, len);
	if (num > 0) {
		if (serial->trace) {
			xsTraceLeft("readBytesUntil", "xsSerial");
			xsTraceLeftBytes(str, num, "xsSerial");
			str[len] = '\0';
			xsTraceLeft(str, "xsSerial");
		}
		xsmcSetString(xsResult, str);
	}

	c_free(str);
}

void modSerialIssueCallback(void *the, void *refcon, uint8_t *message, uint16_t messageLength) {
	modSerial serial = (modSerial)refcon;
	if (messageLength) {
		xsBeginHost(the);
		if (serial->trace) {
			xsTraceLeft("received:", "xsSerial");
			xsTraceLeft(message, "xsSerial");
			xsTraceLeftBytes(message, messageLength, "xsSerial");
		}
		xsCall1_noResult(serial->obj, xsID_onDataReceived, xsString(message));
		xsEndHost(the);
	}
	else {
		modLog("pollCallback - no len\n");
	}
}

void modSerialTriggerCallback(uint8_t *data, int len, void *refcon) {
	modSerial serial = (modSerial)refcon;
//	modMessagePostToMachine(serial->the, data, len, modSerialIssueCallback, serial);
	modSerialIssueCallback(serial->the, serial, data, len);
}
#if gecko
#else
void modSerialTriggerCallback_fromThread(uint8_t *data, int len, void *refcon) {
	modSerial serial = (modSerial)refcon;
	modMessagePostToMachine(serial->the, data, len, modSerialIssueCallback, serial);
}
#endif

void xs_serial_poll(xsMachine *the) {
	modSerial serial = xsmcGetHostData(xsThis);
	int bufSize = 0;

	xsmcVars(1);

	if (xsmcArgc < 1) {
		modSerial_endPoll(serial->device);
		if (serial->buffer) {
			c_free(serial->buffer);
			serial->buffer = NULL;
		}
		modLog("poll stopped\n");
		return;
	}

	if (xsmcHas(xsArg(0), xsID_interval)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_interval);
		serial->interval = xsmcToInteger(xsVar(0));
	}
	else
		serial->interval = MODDEF_SERIAL_SAMPLING_INTERVAL;
	
	if (xsmcHas(xsArg(0), xsID_terminators)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_terminators);
		c_strncpy(serial->terminators, xsmcToString(xsVar(0)), MAX_TERMINATORS);
	}
	else
		strcpy(serial->terminators, "\r\n");

	if (xsmcHas(xsArg(0), xsID_trim)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_trim);
		serial->trim = xsmcToInteger(xsVar(0));
	}
	else
		serial->trim = 0;

	if (xsmcHas(xsArg(0), xsID_chunkSize)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_chunkSize);
		bufSize = xsmcToInteger(xsVar(0));
	}
	else
		bufSize = serial->bufferSize;

	serial->buffer = c_malloc(bufSize);

	modSerial_flush(serial->device);
	modSerial_poll(serial->device, serial->interval, serial->terminators, serial->trim, serial->buffer, bufSize, modSerialTriggerCallback, serial);
}

void xs_serial_destructor(void *data) {
	modSerial serial = (modSerial)data;

	if (serial) {
		if (serial->device)
			modSerial_teardown(serial->device);	

		xsBeginHost(serial->the);
			xsForget(serial->obj);
			xsmcSetHostData(serial->obj, NULL);
		xsEndHost(serial->the);

		if (serial->buffer)
			c_free(serial->buffer);
	}

	if (data)
		c_free(data);

	gSerialInited = 0;
}

void xs_serial(xsMachine *the) {
	modSerial		serial;

	if (gSerialInited)
		return;

	gSerialInited = 1;

	serial = c_calloc(1, sizeof(modSerialRecord));
	if (!serial)
		xsUnknownError("no memory");

	serial->the = the;
	serial->obj = xsThis;

#if MODDEF_SERIAL_TIMEOUT
	serial->timeoutMS = MODDEF_SERIAL_TIMEOUT;
#else
	serial->timeoutMS = 1000;
#endif

#ifdef MODDEF_SERIAL_INTERFACE_UART
	serial->config.interface = MODDEF_SERIAL_INTERFACE_UART;
#endif
	serial->config.interface = 2;

#ifdef MODDEF_SERIAL_BAUD	
	serial->config.baud = MODDEF_SERIAL_BAUD;
#else
	serial->config.baud = 9600;
#endif

#ifdef MODDEF_SERIAL_CONFIG_DATABITS
	serial->config.databits = MODDEF_SERIAL_CONFIG_DATABITS;
#else
	serial->config.databits = 8;
#endif

#ifdef MODDEF_SERIAL_CONFIG_PARITY
	{
		char foo[2] = MODDEF_SERIAL_CONFIG_PARITY;
		serial->config.parity[0] = foo[0];
		serial->config.parity[1] = 0;
	}
#else
	serial->config.parity[0] = 'N';
	serial->config.parity[1] = 0;
#endif

#ifdef MODDEF_SERIAL_CONFIG_STOPBITS
	serial->config.stopbits = MODDEF_SERIAL_CONFIG_STOPBITS;
#else
	serial->config.stopbits = 1;
#endif

#ifdef MODDEF_SERIAL_RX_PORT
	serial->config.rxPort = MODDEF_SERIAL_RX_PORT;
#else
	serial->config.rxPort = 0;
#endif

#ifdef MODDEF_SERIAL_RX_PIN
	serial->config.rxPin = MODDEF_SERIAL_RX_PIN;
#endif

#ifdef MODDEF_SERIAL_TX_PORT
	serial->config.txPort = MODDEF_SERIAL_TX_PORT;
#else
	serial->config.txPort = 0;
#endif

#ifdef MODDEF_SERIAL_TX_PIN
	serial->config.txPin = MODDEF_SERIAL_TX_PIN;
#endif

#ifdef MODDEF_SERIAL_RXBUFFERSIZE
	serial->config.rxBufferSize = MODDEF_SERIAL_RXBUFFERSIZE;
#else
	serial->config.rxBufferSize = 256;
#endif

#ifdef MODDEF_SERIAL_TXBUFFERSIZE
	serial->config.txBufferSize = MODDEF_SERIAL_TXBUFFERSIZE;
#else
	serial->config.txBufferSize = 256;
#endif

#ifdef MODDEF_SERIAL_TRACE
	serial->trace = MODDEF_SERIAL_TRACE;
#else
	serial->trace = 0;
#endif

#ifdef MODDEF_SERIAL_POLLBUFFERSIZE
	serial->bufferSize = MODDEF_SERIAL_POLLBUFFERSIZE;
#else
	serial->bufferSize = 256;
#endif

	xsmcSetHostData(xsThis, serial);
	xsRemember(serial->obj);

	serial->device = modSerialDevice_setup(&(serial->config));
}

