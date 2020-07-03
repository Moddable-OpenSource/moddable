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
#include "qapi_status.h"
#include "qapi_gpioint.h"

struct modDigitalMonitorRecord {
	struct modDigitalMonitorRecord *next;
	xsMachine			*the;
	xsSlot				obj;
	modGPIOConfigurationRecord	config;
	qapi_Instance_Handle_t pH;
//	uint8_t				pin;
	uint8_t				edge;
	uint8_t				triggered;
	uint8_t				externalInterrupt;
	uint32_t			rises;
	uint32_t			falls;
};
typedef struct modDigitalMonitorRecord modDigitalMonitorRecord;
typedef struct modDigitalMonitorRecord *modDigitalMonitor;

static void digitalMonitorISR(qapi_GPIOINT_Callback_Data_t);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modDigitalMonitor gMonitors;
static uint8_t gMonitorCallbackPending;

void xs_digital_monitor_destructor(void *data)
{
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
			if (walker->config.pin == monitor->config.pin)
				activeMonitors++;
			if (walker->next == monitor)
				walker->next = monitor->next;
			walker = walker->next;
		}
	}

	if (activeMonitors == 1) {
		// disable interrupt for this pinA
		qapi_GPIOINT_Deregister_Interrupt(&monitor->pH, monitor->config.pin);
	}

	modCriticalSectionEnd();

	c_free(monitor);
}

void xs_digital_monitor(xsMachine *the)
{
	modDigitalMonitor monitor;
	int pin, edge, mode = kModGPIOInput;
	qapi_Status_t status;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	if (!xsmcHas(xsArg(0), xsID_edge))
		xsUnknownError("edge missing");


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
//	monitor->pin = pin;		// handled in modGPIOInit below
	monitor->edge = edge;
	monitor->triggered = false;
	monitor->rises = 0;
	monitor->falls = 0;

	xsRemember(monitor->obj);

	xsmcSetHostData(xsThis, monitor);

	modGPIOInit(&monitor->config, NULL, pin, mode);

	modCriticalSectionBegin();
	monitor->next = gMonitors;
	gMonitors = monitor;
	modCriticalSectionEnd();

	// install ISR
	switch (edge) {
		case 1: edge = QAPI_GPIOINT_TRIGGER_EDGE_RISING_E; break;
		case 2: edge = QAPI_GPIOINT_TRIGGER_EDGE_FALLING_E; break;
		case 3: edge = QAPI_GPIOINT_TRIGGER_EDGE_DUAL_E; break;
		default:
			xsUnknownError("bad edge");
	}

	status = qapi_GPIOINT_Register_Interrupt(&monitor->pH, monitor->config.pin, digitalMonitorISR, monitor, edge, QAPI_GPIOINT_PRIO_MEDIUM_E, false);
//	status = qapi_GPIOINT_Register_Interrupt(&monitor->pH, monitor->config.pin, digitalMonitorISR, monitor, edge, QAPI_GPIOINT_PRIO_HIGH_E, false);
	qca4020_error("GPIOINT_Register_Interrupt", status);

	// enable interrupt for this pin
	status = qapi_GPIOINT_Enable_Interrupt(&monitor->pH, monitor->config.pin);
	qca4020_error("GPIOINT_Enable_Interrupt", status);

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

void digitalMonitorISR(qapi_GPIOINT_Callback_Data_t data)
{
	uint8_t doUpdate = 0;
	modDigitalMonitor walker = gMonitors;
	modDigitalMonitor monitor = (modDigitalMonitor)data;

// disable GPIO Interrupts
	while (walker) {
//		if (walker->config.pin == monitor->config.pin) {
		if (walker == monitor) {
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
		modMessagePostToMachineFromISR(NULL, digitalMonitorDeliver, NULL);
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

