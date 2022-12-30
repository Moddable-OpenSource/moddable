/*
 * Copyright (c) 2019-2022  Moddable Tech, Inc.
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
	DigitalBank - 

	To do:

		read not blocked on input instances, write not blocked on output instances
		ESP8266 implementation assumes a single VM

*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "hardware/gpio.h"

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
	uint8_t		hasOnReadable;
	uint8_t		isInput;
	uint8_t		useCount;
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

static void digitalISR(uint gpio, uint32_t events);
static void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_digitalbank_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static Digital gDigitals;	// pins with onReadable callbacks
static uint8_t callback_installed = 0;

static const xsHostHooks ICACHE_RODATA_ATTR xsDigitalBankHooks = {
	xs_digitalbank_destructor,
	xs_digitalbank_mark,
	NULL
};

void xs_digitalbank_constructor(xsMachine *the)
{
	Digital digital;
	int mode, pins, rises = 0, falls = 0;
	uint8_t pin;
	uint8_t bank = 0, isInput = 1;
	xsSlot *onReadable;
	xsSlot tmp;

	builtinInitIO();

#if kPinBanks > 1
	if (xsmcHas(xsArg(0), xsID_bank)) {
		uint32_t b;
		xsmcGet(tmp, xsArg(0), xsID_bank);
		b = (uint32_t)xsmcToInteger(tmp);
		if (b >= kPinBanks)
			xsUnknownError("invalid bank");
		bank = (uint8_t)b;
	}
#endif

	xsmcGet(tmp, xsArg(0), xsID_pins);
	pins = xsmcToInteger(tmp);
	if (!pins)
		xsUnknownError("invalid");
	if (!builtinArePinsFree(0, pins))
		xsUnknownError("in use");

	xsmcGet(tmp, xsArg(0), xsID_mode);
	mode = builtinGetSignedInteger(the, &tmp);
	if ((1 == bank) && !((kDigitalInput == mode) || (kDigitalOutput == mode)))
		xsRangeError("invalid mode");
	else if (!(((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)) ||
		(kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)))
		xsRangeError("invalid mode");

	onReadable = builtinGetCallback(the, xsID_onReadable);
	if (0 == bank) {
		if (onReadable) {
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
		}
	}
	else if (1 == bank) {
		if (((kDigitalInput != mode) && (kDigitalOutput != mode)) || onReadable)
			xsRangeError("invalid mode");
	}

	builtinInitializeTarget(the);

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	digital = c_malloc(onReadable ? sizeof(DigitalRecord) : offsetof(DigitalRecord, triggered));
	if (!digital)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, digital);
	digital->pins = 0;
	digital->obj = xsThis;
	digital->useCount = 1;
	xsRemember(digital->obj);

	if (0 == bank) {
		gpio_init_mask(pins);

		for (pin = 0; pin <= 31; pin++) {
			if (!(pins & (1 << (pin & 0x1f))))
				continue;

			switch (mode) {
				case kDigitalInput:
					gpio_set_dir(pin, false);
					break;
				case kDigitalInputPullUp:
					gpio_set_dir(pin, false);
					gpio_set_pulls(pin, true, false);
					break;
				case kDigitalInputPullDown:
					gpio_set_dir(pin, false);
					gpio_set_pulls(pin, false, true);
					break;
				case kDigitalInputPullUpDown:
					gpio_set_dir(pin, false);
					gpio_set_pulls(pin, true, true);
					break;

				case kDigitalOutput:
					gpio_set_dir(pin, true);
					isInput = 0;
					break;
				case kDigitalOutputOpenDrain:
					gpio_set_dir(pin, true);
					isInput = 0;
					break;
			}
		}

		if (onReadable) {
			xsSlot tmp;

			digital->the = the;
			digital->rises = rises;
			digital->falls = falls;
			digital->triggered = 0;

			digital->onReadable = onReadable;

			xsSetHostHooks(xsThis, (xsHostHooks *)&xsDigitalBankHooks);

			builtinCriticalSectionBegin();
				digital->next = gDigitals;
				gDigitals = digital;
			builtinCriticalSectionEnd();

			for (pin = 0; pin <= 31; pin++) {
				uint32_t mask = 1 << (pin & 0x1f);
				if (pins & mask) {
					if (callback_installed)
						gpio_set_irq_enabled(pin,
							((digital->falls & mask) ? GPIO_IRQ_EDGE_FALL : 0)
							| ((digital->rises & mask) ? GPIO_IRQ_EDGE_RISE : 0),
							true);
					else {
						gpio_set_irq_enabled_with_callback(pin,
							((digital->falls & mask) ? GPIO_IRQ_EDGE_FALL : 0)
							| ((digital->rises & mask) ? GPIO_IRQ_EDGE_RISE : 0),
							true, digitalISR);
						callback_installed = 1;
					}
				}
			}
		}
	}
#if WIFI_GPIO
	else {		// bank 1
		pico_use_cyw43();
		switch (mode) {
			case kDigitalInput:
				break;
			case kDigitalOutput:
				isInput = 0;
				break;
		}
	}
#endif

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsDigitalBankHooks);

	digital->pins = pins;
	digital->bank = bank;
	digital->hasOnReadable = onReadable ? 1 : 0;
	digital->isInput = isInput;
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

	if ((0 == digital->bank) && digital->pins) {
		int pin;

		if (digital->hasOnReadable)
			gpio_remove_raw_irq_handler_masked(digital->pins, digitalISR);

		for (pin = 0; pin <= 31; pin++) {
			if (digital->pins & (1 << (pin & 0x1f))) {
				gpio_disable_pulls(pin);
				gpio_deinit(pin);
			}
		}

		builtinFreePins(digital->bank, digital->pins);
	}
#if WIFI_GPIO
	else if (1 == digital->bank && digital->pins) {
		pico_unuse_cyw43();
		builtinFreePins(digital->bank, digital->pins);
	}
#endif

	builtinCriticalSectionEnd();

//	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST))
	if (0 == --digital->useCount)
		c_free(data);
}

void xs_digitalbank_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	Digital digital = it;

	if (digital->hasOnReadable)
		(*markRoot)(the, digital->onReadable);
}

void xs_digitalbank_close(xsMachine *the)
{
	Digital digital = xsmcGetHostData(xsThis);
	if (digital && xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks)) {
		xsForget(digital->obj);
		xs_digitalbank_destructor(digital);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_digitalbank_read(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t result = 0;

	if (!digital->isInput)
		xsUnknownError("unimplemented");

	if (0 == digital->bank) {
		result = gpio_get_all();
		result &= digital->pins;
	}
#if WIFI_GPIO
	else {
		if (cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN))
			result |= (1 << 0);
	}
#endif

	xsmcSetInteger(xsResult, result & digital->pins);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value;

	if (digital->isInput)
		xsUnknownError("unimplemented");

	value = xsmcToInteger(xsArg(0));

	if (0 == digital->bank)
		gpio_put_masked(digital->pins, value);
#if WIFI_GPIO
	else
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value);
#endif
}

void digitalISR(uint gpio, uint32_t events)
{
	Digital walker;
	uint8_t doUpdate = 0;
	uint32_t pin;

	pin = 1 << (gpio & 0x1F);

	for (walker = gDigitals; walker; walker = walker->next) {
		if (!(pin & walker->pins))
			continue;

		uint32_t triggered = walker->triggered;
		walker->triggered |= pin;
		if (!triggered) {
//			__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
			walker->useCount++;
			modMessagePostToMachineFromISR(walker->the, digitalDeliver, walker);
		}
		break;
	}
}

void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital digital = refcon;
	uint32_t triggered;

//	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST)) {
	if (0 == --digital->useCount) {
		c_free(digital);
		return;
	}

	builtinCriticalSectionBegin();
		triggered = digital->triggered;
		digital->triggered = 0;
	builtinCriticalSectionEnd();

	xsBeginHost(digital->the);
		xsmcSetInteger(xsResult, triggered);
		xsCallFunction1(xsReference(digital->onReadable), digital->obj, xsResult);
	xsEndHost(digital->the);
}

