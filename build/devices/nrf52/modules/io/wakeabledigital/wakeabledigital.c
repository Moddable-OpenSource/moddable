/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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
	WakeableDigital - using nRF52 reset reason register

	To do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_* values

#include "nrf_delay.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"

#include "builtinCommon.h"

#define kPinReset (128)

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

struct WakeableDigitalRecord {
	uint8_t		pin;
	uint8_t		hasOnReadable;
	xsMachine	*the;
	xsSlot		target;
	xsSlot		*onReadable;
	struct WakeableDigitalRecord *next;
};
typedef struct WakeableDigitalRecord WakeableDigitalRecord;
typedef struct WakeableDigitalRecord *WakeableDigital;

static void wakeableDigitalDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength);
static uint8_t softdevice_enabled();

void xs_wakeabledigital_constructor(xsMachine *the)
{
	WakeableDigital wd;
	int pin, type;
	uint8_t hasOnReadable;
	xsSlot target;

	xsmcVars(1);

	xsmcGet(target, xsArg(0), xsID_target);
	if (!xsmcTest(target))
		target = xsThis;

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);

	type = xsmcTypeOf(xsVar(0));
	if ((xsNumberType == type) || (xsIntegerType == type)) {
		pin = xsmcToInteger(xsVar(0));
		nrf_gpio_cfg_sense_input(pin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
		nrf_delay_ms(1);
	}
	else if (0 == c_strcmp(xsmcToString(xsVar(0)), "RST"))
		pin = kPinReset;
	else
		xsRangeError("invalid pin");

	hasOnReadable = builtinHasCallback(the, &target, xsID_onReadable);

	wd = c_malloc(hasOnReadable ? sizeof(WakeableDigitalRecord) : offsetof(WakeableDigitalRecord, the));
	if (!wd)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, wd);
	wd->pin = pin;
	wd->hasOnReadable = hasOnReadable;

	if (hasOnReadable) {
		xsSlot tmp;
		wd->the = the;
		wd->target = target;
		xsRemember(wd->target);

		builtinGetCallback(the, &target, xsID_onReadable, &tmp);
		wd->onReadable = xsToReference(tmp);
		//xsRemember(wd->onReadable);

		if (kPinReset == wd->pin)
			modMessagePostToMachine(the, NULL, 0, wakeableDigitalDeliver, wd);
	}
}

void xs_wakeabledigital_destructor(void *data)
{
	WakeableDigital wd = data;
	if (!wd) return;

	if (kPinReset != wd->pin)
		builtinFreePins(1 << wd->pin);

	c_free(wd);
}

void xs_wakeabledigital_close(xsMachine *the)
{
	WakeableDigital wd = xsmcGetHostData(xsThis);
	if (!wd) return;

	xsmcSetHostData(xsThis, NULL);
	if (wd->hasOnReadable) {
		xsForget(wd->target);
		//xsForget(wd->onReadable);
	}
	xs_wakeabledigital_destructor(wd);
}

void xs_wakeabledigital_read(xsMachine *the)
{
	WakeableDigital wd = xsmcGetHostData(xsThis);
	uint32_t reset_reason;

	// get the reset reason
	if (softdevice_enabled())
		sd_power_reset_reason_get(&reset_reason);
	else
		reset_reason = NRF_POWER->RESETREAS;
		
	// clear the reset reason register using the bit mask (required)
	if (softdevice_enabled())
		sd_power_reset_reason_clr(reset_reason);
	else
		NRF_POWER->RESETREAS = reset_reason;

	// Wake up from deep sleep can only occur from digital, analog, or NFC field triggers. Also hard reset.
	if (kResetReasonGPIO == reset_reason || kResetReasonLPCOMP == reset_reason || kResetReasonNFC == reset_reason)
		xsmcSetInteger(xsResult, 1);
	else
		xsmcSetInteger(xsResult, 0);
}

void wakeableDigitalDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	WakeableDigital wd = refcon;
	xsMachine *the = wd->the;

	xsBeginHost(the);
		xsCallFunction0(xsReference(wd->onReadable), wd->target);
	xsEndHost(the);
}

uint8_t softdevice_enabled()
{
#ifdef SOFTDEVICE_PRESENT
	return nrf_sdh_is_enabled();
#else
	return false;
#endif
}
