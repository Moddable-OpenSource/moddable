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
 */

#include "xsmc.h"
#include "xsHost.h"

#include "mc.xs.h"      // for xsID_ values
 
void modRadioPostMessage(void *buffer, uint32_t size);
void modRadioListen(uint32_t mode);
void modRadioInit();
int setMaxSleep(int sleepLevel);

extern void modRadioSleep(void);
extern uint32_t gDeviceUnique;

xsMachine *gRadioMachine = NULL;
xsSlot gRadioInstance;

void xs_Radio(xsMachine *the) {
	if (gRadioMachine)
		xsUnknownError("already instantiated");
	gRadioMachine = the;
	gRadioInstance = xsThis;
	xsmcSetHostData(xsThis, (void *)-1);
	modRadioInit();
	setMaxSleep(1);
}

void xs_Radio_destructor(void *data) {
	if (data)
		gRadioMachine = NULL;
}

void xs_Radio_listen(xsMachine *the) {
	modRadioListen(xsmcToInteger(xsArg(0)));
}

void xs_Radio_waitUntilIdle(xsMachine *the) {
	modRadioWaitUntilIdle();
}

void xs_Radio_setTxPower(xsMachine *the) {
	xsResult = xsInteger(modRadioSetTxPower(xsmcToInteger(xsArg(0))));
}

void xs_Radio_getTxPower(xsMachine *the) {
	xsResult = xsInteger(modRadioGetTxPower());
}

void xs_Radio_getUnique(xsMachine *the) {
	xsResult = xsNumber(gDeviceUnique);
}

void xs_Radio_postMessage(xsMachine *the) {
	int size;
	char *buffer;
	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		size = (int)xsmcGetArrayBufferLength(xsArg(0));
		buffer = c_malloc(size);
		c_memcpy(buffer, xsmcToArrayBuffer(xsArg(0)), size);
	}
	else {
		char *str = xsmcToString(xsArg(0));
		size = c_strlen(str);
		buffer = c_malloc(size);
		c_strcpy(buffer, xsmcToString(xsArg(0)));
	}
	modRadioPostMessage(buffer, size);
}


void modRadioReceivedMessage(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);
		xsmcVars(1);
		xsmcSetArrayBuffer(xsVar(0), message, messageLength);
		xsCall1_noResult(gRadioInstance, xsID_onMessage, xsVar(0));
	xsEndHost(the);
}

void modRadioQueueReceivedMessage(void *message, uint32_t size)
{
	if (message && gRadioMachine)
		modMessagePostToMachine(gRadioMachine, message, size, modRadioReceivedMessage, NULL);
}

