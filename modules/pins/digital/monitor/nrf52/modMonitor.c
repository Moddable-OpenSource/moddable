/*
 * Copyright (c) 2018-2023  Moddable Tech, Inc.
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
#include "modGPIO.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"

typedef struct {
	void			*next;
	xsMachine		*the;
	xsSlot			obj;
	uint8_t			pin;
	uint8_t			triggered;
	uint8_t			closed;
	uint8_t			edge;
	uint32_t		rises;
	uint32_t		falls;
	nrf_drv_gpiote_in_config_t	nrfConfig;
} modDigitalMonitorRecord, *modDigitalMonitor;

typedef struct {
	xsSlot obj;
	xsSlot onWake;
} modDigitalWakeConfigurationRecord, *modDigitalWakeConfiguration;

static void digitalMonitorISR(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void wakeableMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modDigitalMonitor gMonitors = NULL;

void xs_digital_monitor_destructor(void *data)
{
	modDigitalMonitor last = NULL, walker = gMonitors;
	modDigitalMonitor monitor = (modDigitalMonitor)data;

	if (NULL == monitor)
		return;

	// remove from gMonitors list
	while (walker) {
		if (walker == monitor) {
			if (last)
				last->next = walker->next;
			else if (gMonitors == monitor)
				gMonitors = monitor->next;
			break;
		}
		last = walker;
		walker = walker->next;
	}
	
	nrf_drv_gpiote_in_event_disable(monitor->pin);
	nrf_drv_gpiote_in_uninit(monitor->pin);

	c_free(monitor);

	if (!gMonitors) {
		nrf_drv_gpiote_uninit();
	}
}

void xs_digital_monitor(xsMachine *the)
{
	modDigitalMonitor monitor;
	int pin, edge, wakeEdge = 0, mode = kModGPIOInput;
	nrf_gpio_pin_sense_t sense_config = NRF_GPIO_PIN_NOSENSE;
	ret_code_t err_code;
	int woke;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	if (!xsmcHas(xsArg(0), xsID_edge))
		xsUnknownError("edge missing");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	woke = modGPIODidWake(NULL, pin);

	xsmcGet(xsVar(0), xsArg(0), xsID_edge);
	edge = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		mode = xsmcToInteger(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_wakeEdge)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_wakeEdge);
		wakeEdge = xsmcToInteger(xsVar(0));
	}

	monitor = c_calloc(1, sizeof(modDigitalMonitorRecord));
	if (!monitor)
		xsUnknownError("no memory");

	monitor->the = the;
	monitor->obj = xsThis;
	monitor->pin = pin;
	monitor->edge = (uint8_t)edge;
	monitor->triggered = false;
	monitor->rises = 0;
	monitor->falls = 0;
	monitor->next = gMonitors;
	gMonitors = monitor;

	xsRemember(monitor->obj);

	xsmcSetHostData(xsThis, monitor);

	monitor->nrfConfig.is_watcher = false;
	monitor->nrfConfig.hi_accuracy = true;

	if (wakeEdge & kModGPIOWakeRisingEdge)
		sense_config = NRF_GPIO_PIN_SENSE_HIGH;
	else if (wakeEdge & kModGPIOWakeFallingEdge)
		sense_config = NRF_GPIO_PIN_SENSE_LOW;
		
	if (kModGPIOInputPullUp == mode)
		monitor->nrfConfig.pull = NRF_GPIO_PIN_PULLUP;
	else if (kModGPIOInputPullDown == mode)
		monitor->nrfConfig.pull = NRF_GPIO_PIN_PULLDOWN;
	else
		monitor->nrfConfig.pull = NRF_GPIO_PIN_NOPULL;

	if (1 == edge)
		monitor->nrfConfig.sense = NRF_GPIOTE_POLARITY_LOTOHI;	// POSEDGE
	else if (2 == edge)
		monitor->nrfConfig.sense = NRF_GPIOTE_POLARITY_HITOLO;	// NEGEDGE
	else
		monitor->nrfConfig.sense = NRF_GPIOTE_POLARITY_TOGGLE;	// ANYEDGE

	if (!nrf_drv_gpiote_is_init())
		nrf_drv_gpiote_init();

	err_code = nrf_drv_gpiote_in_init(monitor->pin, &monitor->nrfConfig, digitalMonitorISR);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(monitor->pin, true);

	if (NRF_GPIO_PIN_NOSENSE != sense_config)
		nrf_gpio_cfg_sense_set(monitor->pin, sense_config);

	if (xsmcHas(xsArg(0), xsID_onWake)) {
		if (woke) {
			modDigitalWakeConfigurationRecord wake;
			wake.obj = xsThis;
			if (xsmcHas(xsArg(0), xsID_target)) {
				xsmcGet(xsVar(0), xsArg(0), xsID_target);
				xsmcSet(xsThis, xsID_target, xsVar(0));
			}
			xsmcGet(wake.onWake, xsArg(0), xsID_onWake);
			xsRemember(wake.onWake);
			modMessagePostToMachine(the, (uint8_t*)&wake, sizeof(wake), wakeableMonitorDeliver, NULL);
		}
	}
}

void xs_digital_monitor_close(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);
	nrf_gpio_cfg_sense_set(monitor->pin, NRF_GPIO_PIN_NOSENSE);
	nrf_drv_gpiote_in_event_disable(monitor->pin);
	xsForget(monitor->obj);
	monitor->closed = true;
	if (!monitor->triggered)
		xs_digital_monitor_destructor(monitor);
	xsmcSetHostData(xsThis, NULL);
}

void xs_digital_monitor_read(xsMachine *the)
{
	modDigitalMonitor monitor = xsmcGetHostData(xsThis);

	xsmcSetInteger(xsResult, nrf_gpio_pin_read(monitor->pin));
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
	xsUnknownError("unimplemented");
}

void xs_digital_monitor_get_pwm_freq(xsMachine *the)
{
	xsUnknownError("unimplemented");
}
#else
void xs_digital_monitor_get_pwm_duty(xsMachine *the) {}
void xs_digital_monitor_get_pwm_freq(xsMachine *the) {}
#endif

static modDigitalMonitor findMonitor(nrf_drv_gpiote_pin_t pin)
{
	modDigitalMonitor monitor = gMonitors;

	while (monitor) {
		if (monitor->pin == pin)
			break;
		monitor = monitor->next;
	}
	return monitor;
}

void digitalMonitorISR(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	modDigitalMonitor monitor;
	uint8_t value;

	monitor = findMonitor(pin);
	if (!monitor)
		return;

	if (NRF_GPIOTE_POLARITY_LOTOHI == action)
		monitor->rises += 1;
	else
		monitor->falls += 1;

	if (monitor->triggered)
		return;
	monitor->triggered = true;		// only send one notification at a time

	modMessagePostToMachineFromISR(monitor->the, digitalMonitorDeliver, monitor);
}

void digitalMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalMonitor monitor = refcon;

	if (monitor->closed) {
		xs_digital_monitor_destructor(monitor);
		return;
	}

	monitor->triggered = false;		// allow another notification

	xsBeginHost(the);
		xsCall0(monitor->obj, xsID_onChanged);
	xsEndHost(the);
}

void wakeableMonitorDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modDigitalWakeConfiguration wake = (modDigitalWakeConfiguration)message;

	xsBeginHost(the);
		xsCallFunction0(wake->onWake, wake->obj);
		xsForget(wake->onWake);
	xsEndHost(the);
}

