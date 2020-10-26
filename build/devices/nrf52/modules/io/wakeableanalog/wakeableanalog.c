/*
 * Copyright (c) 2020  Moddable Tech, Inc.
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

/*
	WakeableAnalog - using nRF52 reset reason register

	To do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_* values

#include "nrf_delay.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_drv_lpcomp.h"
#include "nrfx_saadc.h"

#include "builtinCommon.h"

#define kPinReset (128)
#define kAnalogResolution 10	// 10 bits

enum {
	kResetReasonUnknown = 0,
	kResetReasonResetPin = (1L << 0),
	kResetReasonWatchdog = (1L << 1),
	kResetReasonSoftReset = (1L << 2),
	kResetReasonLockup = (1L << 3),
	kResetReasonGPIO = (1L << 16),
	kResetReasonLPCOMP = (1L << 17),
	kResetReasonDebugInterface = (1L << 18),
	kResetReasonNFC = (1L << 19)
};

enum {
	kAnalogWakeModeUnknown = -1,
	kAnalogWakeModeCrossing,
	kAnalogWakeModeUp,
	kAnalogWakeModeDown
};

struct WakeableAnalogRecord {
	uint8_t		pin;
	uint8_t		hasOnReadable;
	uint32_t	resetReason;
	xsMachine	*the;
	xsSlot		target;
	xsSlot		*onReadable;
	struct WakeableAnalogRecord *next;
};
typedef struct WakeableAnalogRecord WakeableAnalogRecord;
typedef struct WakeableAnalogRecord *WakeableAnalog;

static void xs_wakeableanalog_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void wakeableAnalogDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength);
static void lpcomp_event_handler(nrf_lpcomp_event_t event);

static const xsHostHooks ICACHE_FLASH_ATTR xsWakeableAnalogHooks = {
	xs_wakeableanalog_destructor,
	xs_wakeableanalog_mark,
	NULL
};

// @@ - ugly
extern void init_analog_channel(int channel);

void xs_wakeableanalog_constructor(xsMachine *the)
{
	WakeableAnalog wa;
	int pin, type, value;
	char *mode;
	uint8_t hasOnReadable;
	xsSlot target;
	uint32_t resetReason;

	xsmcVars(1);

	xsmcGet(target, xsArg(0), xsID_target);
	if (!xsmcTest(target))
		target = xsThis;

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);

	type = xsmcTypeOf(xsVar(0));
	if ((xsNumberType == type) || (xsIntegerType == type)) {
		nrf_drv_lpcomp_config_t config;
		uint16_t reference, detection;
		double scaledValue;
		ret_code_t err_code;
	
		pin = xsmcToInteger(xsVar(0));

		if (!xsmcHas(xsArg(0), xsID_mode))
			xsUnknownError("missing mode");
		if (!xsmcHas(xsArg(0), xsID_value))
			xsUnknownError("missing value");
			
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		mode = xsmcToString(xsVar(0));
		xsmcGet(xsVar(0), xsArg(0), xsID_value);
		value = xsmcToInteger(xsVar(0));
		
		if (0 == c_strcmp("crossing", mode))
			detection = kAnalogWakeModeCrossing;
		else if (0 == c_strcmp("up", mode))
			detection = kAnalogWakeModeUp;
		else if (0 == c_strcmp("down", mode))
			detection = kAnalogWakeModeDown;
		else
			xsRangeError("invalid mode");

		if (pin < NRF_LPCOMP_INPUT_0 || pin > NRF_LPCOMP_INPUT_7)
			xsRangeError("invalid analog channel number");

		scaledValue = ((double)value) / (1L << kAnalogResolution);
		reference = (uint16_t)(scaledValue * (LPCOMP_REFSEL_REFSEL_SupplySevenEighthsPrescaling - LPCOMP_REFSEL_REFSEL_SupplyOneEighthPrescaling + 1));

		config.hal.reference = reference;
		config.hal.detection = detection;
		config.hal.hyst = 0;
		config.input = pin;
		config.interrupt_priority = 6;
		err_code = nrf_drv_lpcomp_init(&config, lpcomp_event_handler);
		if (NRF_SUCCESS != err_code)
			xsUnknownError("wakeable analog config failure");

		nrf_drv_lpcomp_enable();

		nrf_delay_ms(10);	// @@ seems necessary?
	}
	else if (0 == c_strcmp(xsmcToString(xsVar(0)), "RST"))
		pin = kPinReset;
	else
		xsRangeError("invalid pin");

	hasOnReadable = builtinHasCallback(the, &target, xsID_onReadable);

	wa = c_malloc(hasOnReadable ? sizeof(WakeableAnalogRecord) : offsetof(WakeableAnalogRecord, the));
	if (!wa)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, wa);
	wa->pin = pin;
	wa->hasOnReadable = hasOnReadable;

	// get the reset reason
	if (nrf52_softdevice_enabled())
		sd_power_reset_reason_get(&resetReason);
	else
		resetReason = NRF_POWER->RESETREAS;
		
	wa->resetReason = resetReason;
	
	// clear the reset reason register using the bit mask (required)
	if (nrf52_softdevice_enabled())
		sd_power_reset_reason_clr(resetReason);
	else
		NRF_POWER->RESETREAS = resetReason;

	if (hasOnReadable) {
		xsSlot tmp;
		wa->the = the;
		wa->target = target;
		xsRemember(wa->target);

		builtinGetCallback(the, &target, xsID_onReadable, &tmp);
		wa->onReadable = xsToReference(tmp);

		xsSetHostHooks(xsThis, (xsHostHooks *)&xsWakeableAnalogHooks);

		if (kPinReset == wa->pin)
			modMessagePostToMachine(the, NULL, 0, wakeableAnalogDeliver, wa);
	}
}

void xs_wakeableanalog_destructor(void *data)
{
	WakeableAnalog wa = data;
	if (!wa) return;

	if (kPinReset != wa->pin)
		builtinFreePins(1 << wa->pin);

	c_free(wa);
}

void xs_wakeableanalog_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	WakeableAnalog wa = it;

	(*markRoot)(the, wa->onReadable);
}

void xs_wakeableanalog_close(xsMachine *the)
{
	WakeableAnalog wa = xsmcGetHostData(xsThis);
	if (!wa) return;

	xsmcSetHostData(xsThis, NULL);
	xsForget(wa->target);
	xs_wakeableanalog_destructor(wa);
}

void xs_wakeableanalog_read(xsMachine *the)
{
	WakeableAnalog wa = xsmcGetHostData(xsThis);
	uint32_t resetReason = wa->resetReason;

	// Wake up from deep sleep can only occur from digital, analog, or NFC field triggers. Also hard reset.
	if (kPinReset == wa->pin) {
		if (kResetReasonGPIO == resetReason || kResetReasonLPCOMP == resetReason || kResetReasonNFC == resetReason)
			xsmcSetInteger(xsResult, 1);
		else
			xsmcSetInteger(xsResult, 0);
	}
	else {
		nrf_saadc_value_t value;
		init_analog_channel(wa->pin);
		nrfx_saadc_sample_convert(wa->pin, &value);
		xsResult = xsInteger(value);
	}
}

void xs_wakeableanalog_get_wakeup_reason(xsMachine *the)
{
	WakeableAnalog wa = xsmcGetHostData(xsThis);

	switch(wa->resetReason) {
		case kResetReasonUnknown:
			xsmcSetString(xsResult, "unknown");
			break;
		case kResetReasonResetPin:
			xsmcSetString(xsResult, "hard-reset");
			break;
		case kResetReasonWatchdog:
			xsmcSetString(xsResult, "watchdog");
			break;
		case kResetReasonSoftReset:
			xsmcSetString(xsResult, "soft-reset");
			break;
		case kResetReasonLockup:
			xsmcSetString(xsResult, "lockup");
			break;
		case kResetReasonGPIO:
			xsmcSetString(xsResult, "digital");
			break;
		case kResetReasonLPCOMP:
			xsmcSetString(xsResult, "analog");
			break;
		case kResetReasonDebugInterface:
			xsmcSetString(xsResult, "debugger");
			break;
		case kResetReasonNFC:
			xsmcSetString(xsResult, "nfc");
			break;
		default:
			break;
	}
}

void wakeableAnalogDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	WakeableAnalog wa = refcon;
	xsMachine *the = wa->the;

	xsBeginHost(the);
		xsCallFunction0(xsReference(wa->onReadable), wa->target);
	xsEndHost(the);
}

void lpcomp_event_handler(nrf_lpcomp_event_t event)
{
	switch(event) {
		case NRF_LPCOMP_EVENT_READY:
			break;
		case NRF_LPCOMP_EVENT_DOWN:
			break;
		case NRF_LPCOMP_EVENT_UP:
			break;
		case NRF_LPCOMP_EVENT_CROSS:
			break;
	}
}
