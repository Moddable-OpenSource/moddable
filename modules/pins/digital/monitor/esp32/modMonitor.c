/*
 * Copyright (c) 2018-2020  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "modGPIO.h"

#ifndef MODDEF_MONITOR_PWM
	#define MODDEF_MONITOR_PWM 0
#endif

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				pin;
	uint8_t				triggered;
	uint8_t				closed;
	uint8_t				edge;
	uint32_t			rises;
	uint32_t			falls;
#if MODDEF_MONITOR_PWM
	uint32_t			start_off_us;
	uint32_t			start_on_us;
	uint32_t			time_on_us;
	uint32_t			time_off_us;
	uint8_t				pwm_duty;
	uint32_t			pwm_freq;
	uint32_t			pwm_cycle;
#endif
} modDigitalMonitorRecord, *modDigitalMonitor;

#define EDGE_PWM 4
#define ONE_SECOND 1000000

static void digitalMonitorISR(void *refcon);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static uint8_t gISRCount;

void xs_digital_monitor_destructor(void *data)
{
	modDigitalMonitor monitor = data;
	if (NULL == monitor)
		return;

	gpio_set_intr_type(monitor->pin, GPIO_INTR_DISABLE);
	gpio_isr_handler_remove(monitor->pin);
	if (0 == --gISRCount)
		gpio_uninstall_isr_service();

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
	monitor->edge = (uint8_t)edge;
	monitor->triggered = false;
	monitor->closed = false;
	monitor->rises = 0;
	monitor->falls = 0;

#if MODDEF_MONITOR_PWM
	monitor->pwm_duty = monitor->pwm_freq = monitor->pwm_cycle = monitor->start_off_us = monitor->start_on_us = monitor->time_on_us = monitor->time_off_us = 0;
#endif

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

	if (0 == gISRCount++)
		gpio_install_isr_service(0);

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
	monitor->closed = true;
	if (!monitor->triggered)
		xs_digital_monitor_destructor(monitor);
	xsmcSetHostData(xsThis, NULL);
}

void xs_digital_monitor_read(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, gpio_get_level(monitor->pin));
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

#if MODDEF_MONITOR_PWM
void xs_digital_monitor_get_pwm_duty(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, monitor->pwm_duty);
}

void xs_digital_monitor_get_pwm_freq(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, monitor->pwm_freq);
}

#else
void xs_digital_monitor_get_pwm_duty(xsMachine *the) {}
void xs_digital_monitor_get_pwm_freq(xsMachine *the) {}
#endif

void digitalMonitorISR(void *refcon)
{
	modDigitalMonitor monitor = refcon;
	BaseType_t ignore;
	uint8_t value = gpio_get_level(monitor->pin);

#if MODDEF_MONITOR_PWM
	if (monitor->edge == EDGE_PWM) {
		uint32_t now = modMicroseconds();
		if (value) {
			monitor->time_off_us = now - monitor->start_off_us;

			monitor->pwm_cycle = monitor->time_on_us + monitor->time_off_us;
	
			monitor->pwm_freq = ONE_SECOND / monitor->pwm_cycle;
			monitor->pwm_duty = (monitor->time_on_us * 100) / monitor->pwm_cycle;
			monitor->start_on_us = now;
		}
		else 
		{
			monitor->time_on_us = now - monitor->start_on_us;
			monitor->start_off_us = now;
		}
	}
	else
#endif
	{
		if (value)
			monitor->rises += 1;
		else
			monitor->falls += 1;

		if (monitor->triggered)
			return;
		monitor->triggered = true;

		modMessagePostToMachineFromISR(monitor->the, digitalMonitorDeliver, monitor);
	}
}

void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor monitor = refcon;

	if (monitor->closed) {
		xs_digital_monitor_destructor(monitor);
		return;
	}

	monitor->triggered = false;

	xsBeginHost(the);
		xsCall0(monitor->obj, xsID_onChanged);
	xsEndHost(the);
}
