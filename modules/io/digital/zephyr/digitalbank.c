/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

#include "builtinCommon.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

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
	uint8_t		hasOnReadable;
	uint8_t		isInput;
	uint8_t		useCount;
	uint8_t		bank;			// numeric: gpioa == 0, gpiok == 10
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

const struct device *get_gpio_port(const char *name)
{
	if (0 == strcmp("gpioa", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpioa));
	if (0 == strcmp("gpiob", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpiob));
	if (0 == strcmp("gpioc", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpioc));
	if (0 == strcmp("gpiod", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpiod));
	if (0 == strcmp("gpioe", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpioe));
	if (0 == strcmp("gpiof", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpiof));
	if (0 == strcmp("gpiog", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpiog));
	if (0 == strcmp("gpioh", name))
		return DEVICE_DT_GET(DT_NODELABEL(gpioh));
//	if (0 == strcmp("gpioi", name))
//		return DEVICE_DT_GET(DT_NODELABEL(gpioi));
	return NULL;
}

int get_gpio_bank(const char *name)
{
	if (strlen(name) >= 5) {
		int b = name[4] - 'a';
		if (b >= 0 && b <= 10)
			return b;
	}
	return -1;
}

void xs_digitalbank_constructor(xsMachine *the)
{
	Digital digital;
	int mode, pins, rises = 0, falls = 0;
	const struct device *port;
	uint8_t pin;
	uint8_t bank, isInput = 1;
	xsSlot *onReadable;
	xsSlot tmp;
	int err;
	char *portStr;

	builtinInitIO();

	if (!xsmcHas(xsArg(0), xsID_port))
		xsUnknownError("port required");
	xsmcGet(tmp, xsArg(0), xsID_port);
	portStr = xsmcToString(tmp);
	port = get_gpio_port(portStr);
	if (NULL == port)
		xsRangeError("bad port");
	bank = get_gpio_bank(portStr);

	if (xsmcHas(xsArg(0), xsID_pins)) {
		xsmcGet(tmp, xsArg(0), xsID_pins);
		pins = xsmcToInteger(tmp);
	}
	else
		xsUnknownError("invalid");

	if (!builtinArePinsFree(bank, pins))
		xsUnknownError("in use");

	xsmcGet(tmp, xsArg(0), xsID_mode);
	mode = builtinGetSignedInteger(the, &tmp);
	if (!(((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)) ||
		(kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)))
		xsRangeError("invalid mode");

	onReadable = builtinGetCallback(the, xsID_onReadable);
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

	builtinInitializeTarget(the);

	if (kIOFormatNumber != builtinInitializeFormat(the, kIOFormatNumber))
		xsRangeError("invalid format");

	digital = c_malloc(onReadable ? sizeof(DigitalRecord) : offsetof(DigitalRecord, triggered));
	if (!digital)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, digital);
	digital->port = port;
	digital->pins = 0;
	digital->bank = bank;
	digital->obj = xsThis;
	digital->useCount = 1;
	xsRemember(digital->obj);

	for (pin = 0; pin < GPIO_MAX_PINS_PER_PORT; pin++) {
		if (!(pins & (1 << (pin & 0x1f))))
			continue;

		switch (mode) {
			case kDigitalInput:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT);
				break;
			case kDigitalInputPullUp:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_UP);
				break;
			case kDigitalInputPullDown:
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_DOWN);
				break;
			case kDigitalInputPullUpDown:
				//@@ not sure
				err = gpio_pin_configure(digital->port, pin, GPIO_INPUT | GPIO_PULL_UP | GPIO_PULL_DOWN);
				break;

			case kDigitalOutput:
				err = gpio_pin_configure(digital->port, pin, GPIO_OUTPUT);
				isInput = 0;
				break;
			case kDigitalOutputOpenDrain:
				err = gpio_pin_configure(digital->port, pin, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
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

	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST))
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
	int err;

	if (!digital->isInput)
		xsUnknownError("can't read output");

	err = gpio_port_get(digital->port, &result);
	result &= digital->pins;

	xsmcSetInteger(xsResult, result & digital->pins);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value;

	if (digital->isInput)
		xsUnknownError("can't write input");

	value = xsmcToInteger(xsArg(0)) & digital->pins;

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
			__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
			modMessagePostToMachineFromISR(walker->the, digitalDeliver, walker);
		}
		break;
	}
}

void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital digital = refcon;
	uint32_t triggered;

	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST)) {
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


