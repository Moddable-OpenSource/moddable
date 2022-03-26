/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
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
#include "hardware/gpio.h"

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

static void gpioISR(uint gpio, uint32_t events);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modDigitalMonitor gMonitors;

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
	gpio_set_irq_enabled(monitor->pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

	// remove ISR
	if (NULL == gMonitors)
		gpio_set_irq_enabled_with_callback(monitor->pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false, NULL);

	gpio_init(monitor->pin);

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
	if ((pin < 0) || (pin > 29))
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

	// install ISR
	if (NULL == gMonitors)
		gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false, gpioISR);

	modCriticalSectionBegin();
	monitor->next = gMonitors;
	gMonitors = monitor;
	modCriticalSectionEnd();

	// enable interrupt for this pin
	gpio_set_irq_enabled(monitor->pin, edge << 2, true);		// shift to GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE
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

	xsmcSetInteger(xsResult, gpio_get(monitor->pin));
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

void gpioISR(uint gpio, uint32_t events)
{
	modDigitalMonitor walker;
	uint8_t doUpdate = 0;

	for (walker = gMonitors; walker; walker = walker->next) {
		if (walker->pin != gpio)
			continue;

		if (gpio_get(gpio)) {
			if (1 & walker->edge)
				walker->rises += 1;
		}
		else if (2 & walker->edge)
			walker->falls += 1;
		doUpdate = !walker->triggered;
		walker->triggered = true;
		break;
	}

	if (doUpdate)
		modMessagePostToMachineFromISR(walker->the, digitalMonitorDeliver, walker);
}

void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor walker = refcon;

	walker->triggered = false;

	xsBeginHost(walker->the);
		xsCall0(walker->obj, xsID_onChanged);
	xsEndHost(walker->the);
}

void xs_digital_monitor_get_pwm_duty(xsMachine *the) {}
void xs_digital_monitor_get_pwm_freq(xsMachine *the) {}

