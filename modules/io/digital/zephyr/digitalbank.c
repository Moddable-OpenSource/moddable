/*
 * Copyright (c) 2019-2026  Moddable Tech, Inc.
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

*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values
#include "mc.devicetree.h"

#include "builtinCommon.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

#if kModZephyrGPIOBankCount

enum {
	kDigitalInput = 0,
	kDigitalInputPullUp = 1,
	kDigitalInputPullDown = 2,
	kDigitalInputPullUpDown = 3,

	kDigitalOutput = 8,
	kDigitalOutputOpenDrain = 9,

	kDigitalEdgeRising = 1,
	kDigitalEdgeFalling = 2,

	kDigitalActiveLow = 16
};

struct DigitalRecord {
	uint32_t		pins;
	xsSlot		obj;
	uint8_t		hasOnReadable;
	uint8_t		isInput;
	uint8_t		bank;
	atomic_t		useCount;
	const struct device *port;

	// fields after here only allocated if onReadable callback present
	struct gpio_callback callback;
	uint32_t	triggered;
	uint32_t	rises;
	uint32_t	falls;

	xsMachine	*the;
	xsSlot		*onReadable;
	struct DigitalRecord *next;
};
typedef struct DigitalRecord DigitalRecord;
typedef struct DigitalRecord *Digital;

static void digitalISR(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins);
static void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_digitalbank_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static Digital gDigitals;	// pins with onReadable callbacks


/* static */ const xsHostHooks ICACHE_RODATA_ATTR xsDigitalBankHooks = {
	xs_digitalbank_destructor,
	xs_digitalbank_mark,
	NULL
};

void xs_digitalbank_constructor(xsMachine *the)
{
	Digital digital;
	int mode;
	uint32_t pins, rises = 0, falls = 0;
	uint8_t pin;
	uint8_t isInput = 1;
	xsSlot *onReadable;
	xsSlot tmp;
	int err;

	builtinInitIO();

	if (!xsmcHas(xsArg(0), xsID_port))
		xsUnknownError("port required");
	xsmcGet(tmp, xsArg(0), xsID_port);
	const struct modZephyrGPIOBank *bank = modZephyrGetGPIOBank(xsmcToString(tmp));
	if (NULL == bank)
		xsRangeError("bad port");

	if (!xsmcHas(xsArg(0), xsID_pins))
		xsUnknownError("invalid");

	xsmcGet(tmp, xsArg(0), xsID_pins);
	pins = xsmcToUnsigned(tmp);
	if (!builtinArePinsFree(bank->bankIndex, pins))
		xsUnknownError("in use");

	xsmcGet(tmp, xsArg(0), xsID_mode);
	mode = builtinGetSignedInteger(the, &tmp);
	int tmode = mode & ~kDigitalActiveLow;
	if (!(((kDigitalInput <= tmode) && (tmode <= kDigitalInputPullUpDown)) ||
		(kDigitalOutput == tmode) || (kDigitalOutputOpenDrain == tmode)))
		xsRangeError("invalid mode");

	uint8_t activeLow = 0;
	if (mode & kDigitalActiveLow) {
		xsTrace("digital: ActiveLow deprecated, use activeLow: true\n");
		activeLow = 1;
	}
	if (xsmcHas(xsArg(0), xsID_activeLow)) {
		xsmcGet(tmp, xsArg(0), xsID_activeLow);
		activeLow = xsmcTest(tmp);
	}

	uint32_t initialValue = 0;
	if ((kDigitalOutput == tmode) || (kDigitalOutputOpenDrain == tmode)) {
		if (xsmcHas(xsArg(0), xsID_initialValue)) {
			xsmcGet(tmp, xsArg(0), xsID_initialValue);
			initialValue = xsmcToUnsigned(tmp) & pins;
		}
	}

	onReadable = builtinGetCallback(the, xsID_onReadable);
	if (onReadable) {
		if (!((kDigitalInput <= tmode) && (tmode <= kDigitalInputPullUpDown)))
			xsRangeError("invalid mode");

		if (xsmcHas(xsArg(0), xsID_rises)) {
			xsmcGet(tmp, xsArg(0), xsID_rises);
			rises = xsmcToUnsigned(tmp) & pins;
		}
		if (xsmcHas(xsArg(0), xsID_falls)) {
			xsmcGet(tmp, xsArg(0), xsID_falls);
			falls = xsmcToUnsigned(tmp) & pins;
		}

		if (!rises & !falls)
			xsRangeError("invalid edges");
	}

	builtinInitializeTarget(the);

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	digital = c_malloc(onReadable ? sizeof(DigitalRecord) : offsetof(DigitalRecord, triggered));
	if (!digital)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, digital);
	digital->port = bank->device;
	digital->pins = 0;
	digital->bank = bank->bankIndex;
	digital->obj = xsThis;
	atomic_set(&digital->useCount, 1);
	xsRemember(digital->obj);

	int activeLowFlag = activeLow ? GPIO_ACTIVE_LOW : 0;
	for (pin = 0; pin < bank->gpioCount; pin++) {
		if (!(pins & (1 << (pin & 0x1f))))
			continue;

		int initFlag = (initialValue & (1u << (pin & 0x1f))) ? GPIO_OUTPUT_INIT_HIGH : GPIO_OUTPUT_INIT_LOW;
		switch (tmode) {
			case kDigitalInput:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | activeLowFlag);
				break;
			case kDigitalInputPullUp:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_UP | activeLowFlag);
				break;
			case kDigitalInputPullDown:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_DOWN | activeLowFlag);
				break;
			case kDigitalInputPullUpDown:
				//@@ not sure
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_UP | GPIO_PULL_DOWN | activeLowFlag);
				break;

			case kDigitalOutput:
				err = gpio_pin_configure(digital->port, pin, GPIO_OUTPUT | initFlag | activeLowFlag);
				isInput = 0;
				break;
			case kDigitalOutputOpenDrain:
				err = gpio_pin_configure(digital->port, pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN | initFlag | activeLowFlag);
				isInput = 0;
				break;
		}
	}

	if (onReadable) {
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

		gpio_init_callback(&digital->callback, digitalISR, pins);
		gpio_add_callback(digital->port, &digital->callback);
		for (pin = 0; pin < GPIO_MAX_PINS_PER_PORT; pin++) {
			uint32_t mask = 1 << (pin & 0x1f);
			uint32_t flags = 0;
			if (pins & mask) {
				if ((rises & mask) && (falls & mask))
					flags = GPIO_INT_EDGE_BOTH;
				else if (rises & mask)
					flags = GPIO_INT_EDGE_RISING;
				else if (falls & mask)
					flags = GPIO_INT_EDGE_FALLING;
				else
					flags = GPIO_INT_DISABLE;
				gpio_pin_interrupt_configure(digital->port, pin, flags);
			}
		}
	}

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsDigitalBankHooks);

	digital->pins = pins;
	digital->hasOnReadable = onReadable ? 1 : 0;
	digital->isInput = isInput;
	builtinUsePins(digital->bank, pins);
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

	builtinCriticalSectionEnd();

	if (digital->pins) {
		int pin;

		gpio_remove_callback(digital->port, &digital->callback);
		for (pin = 0; pin <= GPIO_MAX_PINS_PER_PORT; pin++) {
			if (digital->pins & (1 << (pin & 0x1f))) {
				if (digital->isInput)
					gpio_pin_interrupt_configure(digital->port, pin, GPIO_INT_DISABLE);
				gpio_pin_configure(digital->port, pin, GPIO_DISCONNECTED);
			}
		}

		builtinFreePins(digital->bank, digital->pins);
	}

	if (1 == atomic_dec(&digital->useCount))
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
	uint32_t result;

	if (!digital->isInput)
		xsUnknownError("can't read output");

	gpio_port_get(digital->port, &result);
	xsmcSetUnsigned(xsResult, result & digital->pins);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value;

	if (digital->isInput)
		xsUnknownError("can't write input");

	value = xsmcToInteger(xsArg(0));
	gpio_port_set_masked(digital->port, digital->pins, value);
}

void digitalISR(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	Digital walker;

	for (walker = gDigitals; walker; walker = walker->next) {
		if ((port != walker->port) || !(pins & walker->pins))
			continue;

		uint32_t triggered = walker->triggered;
		walker->triggered |= pins;
		if (!triggered) {
			atomic_inc(&walker->useCount);
			modMessagePostToMachineFromISR(walker->the, digitalDeliver, walker);
		}
		break;
	}
}

void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital digital = refcon;
	uint32_t triggered;

	if (1 == atomic_dec(&digital->useCount)) {
		c_free(digital);
		return;
	}

	builtinCriticalSectionBegin();
		triggered = digital->triggered;
		digital->triggered = 0;
	builtinCriticalSectionEnd();

	xsBeginHost(digital->the);
		xsmcSetUnsigned(xsResult, triggered);
		xsCallFunction1(xsReference(digital->onReadable), digital->obj, xsResult);
	xsEndHost(digital->the);
}

uint32_t modDigitalBankRead(Digital digital)
{
	uint32_t result;

	gpio_port_get(digital->port, &result);
	return result &= digital->pins;
}

void modDigitalBankWrite(Digital digital, uint32_t value)
{
	gpio_port_set_masked(digital->port, digital->pins, value);
}

#else // !kModZephyrGPIOBankCount

void xs_digitalbank_constructor(xsMachine *the)
{
	xsUnknownError("no GPIO");
}

void xs_digitalbank_destructor(void *) {}
void xs_digitalbank_close(xsMachine *the) {}
void xs_digitalbank_read(xsMachine *the) {}
void xs_digitalbank_write(xsMachine *the) {}

#endif
