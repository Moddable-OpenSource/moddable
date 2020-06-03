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
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values

#include "modGPIO.h"
#include "gpiointerrupt/inc/gpiointerrupt.h"

struct modDigitalMonitorRecord {
	struct modDigitalMonitorRecord *next;
	xsMachine			*the;
	xsSlot				obj;
	modGPIOConfigurationRecord	config;
	uint8_t				pin;
	uint8_t				edge;
	uint8_t				triggered;
	uint8_t				externalInterrupt;
	uint32_t			rises;
	uint32_t			falls;
};
typedef struct modDigitalMonitorRecord modDigitalMonitorRecord;
typedef struct modDigitalMonitorRecord *modDigitalMonitor;

static void digitalMonitorISR(uint8_t pin);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modDigitalMonitor gMonitors;
static uint8_t gMonitorCallbackPending;

void xs_digital_monitor_destructor(void *data)
{
	modCriticalSectionDeclare;
	modDigitalMonitor monitor = data;
	uint8_t activeMonitors = 0;
	if (NULL == monitor)
		return;

	modCriticalSectionBegin();

	if (gMonitors == monitor)
		gMonitors = monitor->next;
	else {
		modDigitalMonitor walker = gMonitors;
		while (walker) {
			if (walker->pin == monitor->pin)
				activeMonitors++;
			if (walker->next == monitor)
				walker->next = monitor->next;
			walker = walker->next;
		}
	}

	if (activeMonitors == 1) {
		// disable interrupt for this pin
		GPIO_IntDisable(1 << monitor->config.pin);
		GPIOINT_CallbackUnRegister(monitor->config.pin);
	}

	modCriticalSectionEnd();

	c_free(monitor);
}

void xs_digital_monitor(xsMachine *the)
{
	modCriticalSectionDeclare;
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
	if ((edge < 1) || (edge > 3))
		xsUnknownError("invalid edge");

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		mode = xsmcToInteger(xsVar(0));
	}

	monitor = c_malloc(sizeof(modDigitalMonitorRecord));
	if (!monitor)
		xsUnknownError("no memory");

	monitor->the = the;
	monitor->obj = xsThis;
	monitor->pin = pin;
	monitor->edge = edge;
	monitor->triggered = false;
	monitor->rises = 0;
	monitor->falls = 0;

	xsRemember(monitor->obj);

	xsmcSetHostData(xsThis, monitor);

	modCriticalSectionBegin();

	modGPIOInit(&monitor->config, (char*)port, pin, mode);

	// install ISR
	GPIOINT_CallbackRegister(pin, digitalMonitorISR);

	monitor->next = gMonitors;
	gMonitors = monitor;

	// enable interrupt for this pin
	GPIO_ExtIntConfig(port, pin, pin, // pin >> 2,
		(edge != 2) ? true : false, 	// rising edge (pos or any)
		(edge != 1) ? true : false,		// falling edge (neg or any)
		true);							// enable
	GPIO_IntEnable(pin);

	modCriticalSectionEnd();
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

void xs_digital_monitor_get_rises(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	if (!(monitor->edge & 1))
		xsUnknownError("not configured");

	xsmcSetInteger(xsResult, monitor->rises);
}

void xs_digital_monitor_get_falls(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	if (!(monitor->edge & 2))
		xsUnknownError("not configured");

	xsmcSetInteger(xsResult, monitor->falls);
}

void digitalMonitorISR(uint8_t pin)
{
	uint8_t doUpdate = 0;
	modDigitalMonitor walker = gMonitors;

// disable GPIO Interrupts
	while (walker) {
		if (walker->pin == pin) {
			if (modGPIORead(&walker->config)) {
				if (1 & walker->edge) {
					walker->rises += 1;
					walker->triggered = true;
					doUpdate = 1;
				}
			}
			else if (2 & walker->edge) {
				walker->falls += 1;
				walker->triggered = true;
				doUpdate = 1;
			}
		}
		walker = walker->next;
	}
// enable GPIO Interrupts

	if (doUpdate && !gMonitorCallbackPending) {
		gMonitorCallbackPending = true;
		modMessagePostToMachineFromPool(NULL, digitalMonitorDeliver, NULL);
	}
}

void digitalMonitorDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor walker;

	gMonitorCallbackPending = false;

	for (walker = gMonitors; walker; walker = walker->next) {
		if (!walker->triggered)
			continue;

		walker->triggered = false;

		xsBeginHost(walker->the);
			xsCall0(walker->obj, xsID_onChanged);
		xsEndHost(walker->the);
	}
}

void xs_digital_monitor_get_pwm_duty(xsMachine *the) {}
void xs_digital_monitor_get_pwm_freq(xsMachine *the) {}

