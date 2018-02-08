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
#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "modGPIO.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				pin;
	uint8_t				triggered;
	uint32_t			triggerCount;
} modDigitalMonitorRecord, *modDigitalMonitor;

static void digitalMonitorISR(void *refcon);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static uint8_t gISRCount;

static SemaphoreHandle_t gMutex;

void xs_digital_monitor_destructor(void *data)
{
	modDigitalMonitor monitor = data;
	if (NULL == monitor)
		return;

	gpio_set_intr_type(monitor->pin, GPIO_INTR_DISABLE);
	gpio_isr_handler_remove(monitor->pin);
	if (0 == --gISRCount) {
		gpio_uninstall_isr_service();
		vSemaphoreDelete(gMutex);
		gMutex = NULL;
	}

	c_free(monitor);
}

void xs_digital_monitor(xsMachine *the)
{
	modDigitalMonitor monitor;
	int pin, edge, mode = kModGPIOInput;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	if (!xsmcHas(xsArg(0), xsID_edge))
		xsUnknownError("edge missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_edge);
	edge = xsmcToInteger(xsVar(0));

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
	monitor->triggered = false;
	monitor->triggerCount = 0;

	xsRemember(monitor->obj);

	xsmcSetHostData(xsThis, monitor);

	gpio_pad_select_gpio(pin);
	gpio_set_direction(pin, GPIO_MODE_INPUT);

	if (kModGPIOInputPullUp == mode)
		gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
	else if (kModGPIOInputPullDown == mode)
		gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
	else if (kModGPIOInputPullUpDown == mode)
		gpio_set_pull_mode(pin, GPIO_PULLUP_PULLDOWN);
	else
		gpio_set_pull_mode(pin, GPIO_FLOATING);

	if (0 == gISRCount++) {
		gMutex = xSemaphoreCreateMutex();
		gpio_install_isr_service(0);
	}

	if (1 == edge)
		gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
	else if (2 == edge)
		gpio_set_intr_type(pin, GPIO_INTR_NEGEDGE);
	else
		gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);

	gpio_isr_handler_add(pin, digitalMonitorISR, monitor);
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

	xsmcSetInteger(xsResult, gpio_get_level(monitor->pin));
}

void xs_digital_monitor_get_count(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, monitor->triggerCount);
}

void digitalMonitorISR(void *refcon)
{
	modDigitalMonitor monitor = refcon;
	BaseType_t ignore;

	xSemaphoreTakeFromISR(gMutex, &ignore);
	monitor->triggerCount += 1;
	if (monitor->triggered) {
		xSemaphoreGiveFromISR(gMutex, &ignore);
		return;
	}
	monitor->triggered = true;
	xSemaphoreGiveFromISR(gMutex, &ignore);

	modMessagePostToMachineFromISR(monitor->the, digitalMonitorDeliver, monitor);
}

void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor monitor = refcon;

	xSemaphoreTake(gMutex, portMAX_DELAY);
		monitor->triggered = false;
	xSemaphoreGive(gMutex);

	xsBeginHost(the);
		xsCall0(monitor->obj, xsID_onChanged);
	xsEndHost(the);
}
