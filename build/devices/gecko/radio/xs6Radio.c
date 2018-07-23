/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xsmc.h"
#include "xsgecko.h"
#include "xsPlatform.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"

#include "mc.xs.h"      // for xsID_ values
    
#define ID(symbol) (xsID_##symbol)

void geckoPostMessage(void *buffer, uint32_t size);
void geckoSetRadioListen(uint32_t mode);
extern uint32_t gDeviceUnique;

xsMachine *gRadioMachine;

void xs_Radio(xsMachine *the) {
	gRadioMachine = the;
}

void xs_Radio_destructor(void *data) {
	if (data)
		c_free(data);
}

void xs_Radio_listen(xsMachine *the) {
	geckoSetRadioListen(xsmcToInteger(xsArg(0)));
}

void xs_Radio_waitUntilIdle(xsMachine *the) {
	geckoWaitUntilIdle();
}

void xs_Radio_setTxPower(xsMachine *the) {
	xsResult = xsInteger(geckoSetTxPower(xsmcToInteger(xsArg(0))));
}

void xs_Radio_getTxPower(xsMachine *the) {
	xsResult = xsInteger(geckoGetTxPower());
}

void xs_Radio_getUnique(xsMachine *the) {
	xsResult = xsNumber(gDeviceUnique);
}

void xs_Radio_postMessage(xsMachine *the) {
	int size;
	char *buffer;
	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		size = (int)xsGetArrayBufferLength(xsArg(0));
		buffer = c_malloc(size);
		c_memcpy(buffer, xsmcToArrayBuffer(xsArg(0)), size);
	}
	else {
		char *str = xsmcToString(xsArg(0));
		size = c_strlen(str);
		buffer = c_malloc(size);
		c_strcpy(buffer, xsmcToString(xsArg(0)));
	}
	geckoPostMessage(buffer, size);
}

void fromGeckoMessage(modTimer timer, void *message, uint32_t size)
{
	xsBeginHost(gRadioMachine);
		xsmcVars(1);
		xsVar(0) = xsArrayBuffer(message, size);
		(void)xsCall1(xsGlobal, ID(onMessage), xsVar(0));
	xsEndHost(gRadioMachine);
}

void queueGeckoReceivedMessage(void *message, uint32_t size) {
	if (message) {
		modTimerAdd(0, 0, fromGeckoMessage, message, size);
	}
}
