/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "xsgecko.h"
#include "mc.xs.h"			// for xsID_ values

#include "modGPIO.h"
#include "gpiointerrupt/inc/gpiointerrupt.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	modGPIOConfigurationRecord	config;
	uint8_t				pin;
	uint8_t				externalInterrupt;
	uint8_t				triggered;
	uint32_t			triggerCount;
} modDigitalMonitorRecord, *modDigitalMonitor;

static void digitalMonitorISR(uint8_t pin);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

modDigitalMonitor gMonitors[16] = {NULL};

//static uint8_t gISRCount;
//static SemaphoreHandle_t gMutex;

void xs_digital_monitor_destructor(void *data)
{
	modDigitalMonitor monitor = data;
	if (NULL == monitor)
		return;

	GPIO_IntDisable(1 << monitor->config.pin);
	gMonitors[monitor->config.pin] = NULL;
	GPIOINT_CallbackUnRegister(monitor->config.pin);

	c_free(monitor);
}

void xs_digital_monitor(xsMachine *the)
{
	modDigitalMonitor monitor;
	int pin, port, edge, mode = kModGPIOInput;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	if (!xsmcHas(xsArg(0), xsID_edge))
		xsUnknownError("edge missing");

	if (xsmcHas(xsArg(0), xsID_port)) {
		char *str;
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		str = xsmcToString(xsVar(0));
		port = str[8] - 'A';
	}
	else {
		xsUnknownError("port missing");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_edge);
	edge = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		mode = xsmcToInteger(xsVar(0));
	}

	if (gMonitors[pin])
		xsUnknownError("pin already monitored");

	monitor = c_malloc(sizeof(modDigitalMonitorRecord));
	if (!monitor)
		xsUnknownError("no memory");

	monitor->the = the;
	monitor->obj = xsThis;
	monitor->triggered = false;
	monitor->triggerCount = 0;

	xsRemember(monitor->obj);

	xsmcSetHostData(xsThis, monitor);

	gMonitors[pin] = monitor;

	modGPIOInit(&monitor->config, (char*)port, pin, mode);

	GPIOINT_CallbackRegister(pin, digitalMonitorISR);
	GPIO_ExtIntConfig(port, pin, pin, // pin >> 2,
		(edge != 2) ? true : false, 	// rising edge (pos or any)
		(edge != 1) ? true : false,		// falling edge (neg or any)
		true);							// enable
	GPIO_IntEnable(pin);
}

void xs_digital_monitor_close(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);
	xsForget(monitor->obj);
	xs_digital_monitor_destructor(monitor);
	xsmcSetHostData(xsThis, NULL);
}

void xs_digital_monitor_read(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, modGPIORead(&monitor->config));
}

void xs_digital_monitor_get_count(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, monitor->triggerCount);
}

void digitalMonitorISR(uint8_t pin)
{
	modDigitalMonitor monitor = gMonitors[pin];

	monitor->triggerCount += 1;
	monitor->triggered = true;

	modMessagePostToMachineFromPool(monitor->the, digitalMonitorDeliver, monitor);
}

void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor monitor = refcon;

	monitor->triggered = false;

	xsBeginHost(the);
		xsCall0(monitor->obj, xsID_onChanged);
	xsEndHost(the);
}
