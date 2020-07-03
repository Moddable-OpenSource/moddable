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

#include "xsmc.h"
#include "xsHost.h"
#include "modGPIO.h"
#include "mc.xs.h"			// for xsID_ values
#include "ets_sys.h"

struct modDigitalMonitorRecord {
	struct modDigitalMonitorRecord *next;
	xsMachine	*the;
	xsSlot		obj;
	uint8_t		pin;
	uint8_t		edge;
	uint8_t		triggered;
	uint32_t	rises;
	uint32_t	falls;
};
typedef struct modDigitalMonitorRecord modDigitalMonitorRecord;
typedef struct modDigitalMonitorRecord *modDigitalMonitor;

static void digitalMonitorISR(void *refcon);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modDigitalMonitor gMonitors;
static uint8_t gMonitorCallbackPending;

void xs_digital_monitor_destructor(void *data)
{
	modDigitalMonitor monitor = data;
	if (NULL == monitor)
		return;

	modCriticalSectionBegin();

	if (gMonitors == monitor)
		gMonitors = monitor->next;
	else {
		modDigitalMonitor walker;
		for (walker = gMonitors; walker; walker = walker->next) {
			if (walker->next == monitor) {
				walker->next = monitor->next;
				break;
			}
		}
	}

	// disable interrupt for this pin
	GPC(monitor->pin) &= ~(0xF << GPCI);
	GPIEC = (1 << monitor->pin);

	// remove ISR
	if (NULL == gMonitors)
		ETS_GPIO_INTR_DISABLE();

	modCriticalSectionEnd();

	c_free(monitor);
}

void xs_digital_monitor(xsMachine *the)
{
	modDigitalMonitor monitor;
	int pin, edge, mode = kModGPIOInput;
	modGPIOConfigurationRecord config;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	if (!xsmcHas(xsArg(0), xsID_edge))
		xsUnknownError("edge missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));
	if ((pin < 0) || (pin > 15))
		xsUnknownError("invalid pin");

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

	config.pin = (uint8_t)pin;
	modGPIOSetMode(&config, mode);

	modCriticalSectionBegin();

	// install ISR
	if (NULL == gMonitors) {
		ETS_GPIO_INTR_ATTACH(digitalMonitorISR, NULL);
		ETS_GPIO_INTR_ENABLE();
	}
	monitor->next = gMonitors;
	gMonitors = monitor;

	// enable interrupt for this pin
	GPC(pin) &= ~(0xF << GPCI);
	GPIEC = (1 << pin);
	GPC(pin) |= ((edge & 0xF) << GPCI);

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

	xsmcSetInteger(xsResult, (GPIO_REG_READ(GPIO_IN_ADDRESS) >> monitor->pin) & 1);
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

void ICACHE_RAM_ATTR digitalMonitorISR(void *ignore)
{
	uint32_t status = GPIE;
	GPIEC = status;				// clear interrupts
	if (!status)
		return;

	ETS_GPIO_INTR_DISABLE();
	uint32_t levels = GPI;
	uint8_t pin = 0;
	uint8_t doUpdate = 0;
	while (status) {
		if (1 & status) {
			modDigitalMonitor walker;
			for (walker = gMonitors; walker; walker = walker->next) {
				if (walker->pin != pin)
					continue;

				if (1 & levels) {
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
		}
		pin += 1;
		status >>= 1;
		levels >>= 1;
	}
	ETS_GPIO_INTR_ENABLE();

	if (doUpdate && !gMonitorCallbackPending) {
		gMonitorCallbackPending = true;
		modMessagePostToMachineFromPool(NULL, digitalMonitorDeliver, NULL);		// N.B. no THE required on ESP8266 since it is single threaded... would be unsafe on ESP32
	}
}

void digitalMonitorDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor walker;

	gMonitorCallbackPending = false;

//@@ bad things happen if a monitor is removed inside this loop
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

