/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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
	DigitalBank - uing ESP8266 hardware registers and ESP32 hybrid of ESP-IDF and hardware registers

	To do:

		read not blocked on input instances, write not blocked on output instances
		ESP8266 implementation assumes a single VM

*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/gpio.h"

#include "soc/gpio_caps.h"
#include "soc/gpio_periph.h"
#include "hal/gpio_hal.h"

enum {
	kDigitalInput = 0,
	kDigitalInputPullUp = 1,
	kDigitalInputPullDown = 2,
	kDigitalInputPullUpDown = 3,

	kDigitalOutput = 8,
	kDigitalOutputOpenDrain = 9,

	kDigitalEdgeRising = 1,
	kDigitalEdgeFalling = 2,
};

struct DigitalRecord {
	uint32_t	pins;
	xsSlot		obj;
	uint8_t		bank;
	// fields after here only allocated if onReadable callback present
	uint32_t	triggered;
	uint32_t	rises;
	uint32_t	falls;

	xsMachine	*the;
	xsSlot		*onReadable;
	struct DigitalRecord *next;
};
typedef struct DigitalRecord DigitalRecord;
typedef struct DigitalRecord *Digital;

static void digitalISR(void *refcon);
static void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_digitalbank_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static Digital gDigitals;	// pins with onReadable callbacks

static const xsHostHooks ICACHE_RODATA_ATTR xsDigitalBankHooks = {
	xs_digitalbank_destructor,
	xs_digitalbank_mark,
	NULL
};

void xs_digitalbank_constructor(xsMachine *the)
{
	Digital digital;
	int hasOnReadable = 0, mode, pins, rises = 0, falls = 0;
	uint8_t pin;
	uint8_t bank = 0;
	xsSlot tmp;

#if kPinBanks > 1
	if (xsmcHas(xsArg(0), xsID_bank)) {
		uint32_t b;
		xsmcGet(tmp, xsArg(0), xsID_bank);
		b = xsmcToInteger(tmp);
		if (b >= kPinBanks)
			xsUnknownError("invalid bank");
		bank = (uint8_t)b;
	}
#endif

	xsmcGet(tmp, xsArg(0), xsID_pins);
	pins = xsmcToInteger(tmp);
	if (!builtinArePinsFree(bank, pins))
		xsUnknownError("in use");

	xsmcGet(tmp, xsArg(0), xsID_mode);
	mode = xsmcToInteger(tmp);
	if (!(((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)) ||
		(kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)))
		xsRangeError("invalid mode");

	if (builtinHasCallback(the, xsID_onReadable)) {
		if (!((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)))
			xsRangeError("invalid mode");

		if (xsmcHas(xsArg(0), xsID_rises)) {
			xsmcGet(tmp, xsArg(0), xsID_rises);
			rises = xsmcToInteger(tmp) & pins;
		}
		if (xsmcHas(xsArg(0), xsID_falls)) {
			xsmcGet(tmp, xsArg(0), xsID_falls);
			falls = xsmcToInteger(tmp) & pins;
		}

		if (!rises & !falls)
			xsRangeError("invalid edges");

		hasOnReadable = 1;
	}

	builtinInitializeTarget(the);

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	if (bank && (~3 & pins) && ((kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)))
		xsRangeError("invalid mode");		// input-only pins

	digital = c_malloc(hasOnReadable ? sizeof(DigitalRecord) : offsetof(DigitalRecord, triggered));
	if (!digital)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, digital);
	digital->pins = 0;
	digital->obj = xsThis;
	xsRemember(digital->obj);

	int lastPin = bank ? GPIO_NUM_MAX - 1 : 31;
	for (pin = bank ? 32 : 0; pin <= lastPin; pin++) {
		if (!(pins & (1 << (pin & 0x1f))))
			continue;

		gpio_pad_select_gpio(pin);
		switch (mode) {
			case kDigitalInput:
			case kDigitalInputPullUp:
			case kDigitalInputPullDown:
			case kDigitalInputPullUpDown:
				gpio_set_direction(pin, GPIO_MODE_INPUT);
				if (kDigitalInputPullUp == mode)
					gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
				else if (kDigitalInputPullDown == mode)
					gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
				else if (kDigitalInputPullUpDown == mode)
					gpio_set_pull_mode(pin, GPIO_PULLUP_PULLDOWN);
				else
					gpio_set_pull_mode(pin, GPIO_FLOATING);
				break;

			case kDigitalOutput:
			case kDigitalOutputOpenDrain:
				gpio_set_level(pin, 0);
				gpio_set_direction(pin, (kDigitalOutputOpenDrain == mode) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT);
				break;
		}
	}

	if (hasOnReadable) {
		xsSlot tmp;

		digital->the = the;
		digital->rises = rises;
		digital->falls = falls;
		digital->triggered = 0;
// exception for rise/fall on pin 16
		builtinGetCallback(the, xsID_onReadable, &tmp);
		digital->onReadable = xsToReference(tmp);

		xsSetHostHooks(xsThis, (xsHostHooks *)&xsDigitalBankHooks);

			if (NULL == gDigitals)
				gpio_install_isr_service(0);

			builtinCriticalSectionBegin();
				digital->next = gDigitals;
				gDigitals = digital;
			builtinCriticalSectionEnd();

			for (pin = bank ? 32 : 0; pin <= lastPin; pin++) {
				uint32_t mask = 1 << (pin & 0x1f);
				if (pins & mask) {
					gpio_isr_handler_add(pin, digitalISR, (void *)(uintptr_t)pin);
					gpio_intr_enable(pin);

					if ((rises & mask) && (falls & mask))
						gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
					else if (rises & mask)
						gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
					else if (falls & mask)
						gpio_set_intr_type(pin, GPIO_INTR_NEGEDGE);
					else
						gpio_set_intr_type(pin, GPIO_INTR_DISABLE);
				}
			}
	}

	digital->pins = pins;
	digital->bank = bank;
	builtinUsePins(bank, pins);
}

void xs_digitalbank_destructor(void *data)
{
	Digital digital = data;
	if (!digital) return;

	builtinCriticalSectionBegin();

	if (gDigitals == digital)
		gDigitals = digital->next;
	else {
		Digital walker;
		for (walker = gDigitals; walker; walker = walker->next) {
			if (walker->next == digital) {
				walker->next = digital->next;
				break;
			}
		}
	}

	if (digital->pins) {
		int pin, lastPin = digital->bank ? GPIO_NUM_MAX - 1 : 31;

		for (pin = digital->bank ? 32 : 0; pin <= lastPin; pin++) {
			if (digital->pins & (1 << (pin & 0x1f))) {
				gpio_isr_handler_remove(pin);
				gpio_reset_pin(pin);
			}
		}

		if (NULL == gDigitals)
			gpio_uninstall_isr_service();
		builtinFreePins(digital->bank, digital->pins);
	}

	builtinCriticalSectionEnd();

	c_free(data);
}

void xs_digitalbank_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	Digital digital = it;

	(*markRoot)(the, digital->onReadable);
}

void xs_digitalbank_close(xsMachine *the)
{
	Digital digital = xsmcGetHostData(xsThis);
	if (!digital) return;

	xsmcSetHostData(xsThis, NULL);
	xsForget(digital->obj);
	xs_digitalbank_destructor(digital);
}

void xs_digitalbank_read(xsMachine *the)
{
	Digital digital = xsmcGetHostData(xsThis);
	uint32_t result;

	if (!digital)
		xsUnknownError("bad state");

	gpio_dev_t *hw = &GPIO;

    if (digital->bank)
        result = hw->in1.data & digital->pins;
    else
        result = hw->in & digital->pins;

	xsmcSetInteger(xsResult, result);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostData(xsThis);
	uint32_t value = xsmcToInteger(xsArg(0)) & digital->pins;

	if (!digital)
		xsUnknownError("bad state");

	gpio_dev_t *hw = &GPIO;

	if (digital->bank) {
		hw->out1_w1ts.data = value;
		hw->out1_w1tc.data = ~value & digital->pins;
	}
	else {
		hw->out_w1ts = value;
		hw->out_w1tc = ~value & digital->pins;
	}
}

void IRAM_ATTR digitalISR(void *refcon)
{
	uint32_t pin = (uintptr_t)refcon;
	uint8_t bank = (pin >> 5) & 1;
	Digital walker;

	pin = 1 << (pin & 0x1F);

	for (walker = gDigitals; walker; walker = walker->next) {
		if ((bank != walker->bank) || !(pin & walker->pins))
			continue;

		uint32_t triggered = walker->triggered;
		walker->triggered |= pin;
		if (!triggered)
			modMessagePostToMachineFromISR(walker->the, digitalDeliver, walker);
		break;
	}
}

void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital digital = refcon;
	uint32_t triggered;

	builtinCriticalSectionBegin();
		triggered = digital->triggered;
		digital->triggered = 0;
	builtinCriticalSectionEnd();

	xsBeginHost(digital->the);
		xsmcSetInteger(xsResult, triggered);
		xsCallFunction1(xsReference(digital->onReadable), digital->obj, xsResult);
	xsEndHost(digital->the);
}

