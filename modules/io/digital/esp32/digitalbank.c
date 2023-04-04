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

		ESP8266 implementation assumes a single VM

*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include "driver/gpio.h"

//#include "soc/gpio_caps.h"
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

static void digitalISR(void *refcon);
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
	int mode, pins, rises = 0, falls = 0;
	uint8_t pin;
	uint8_t bank = 0, isInput = 1;
	xsSlot *onReadable = NULL;
	xsSlot tmp;
	uint32_t mask;

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
	mask = bank ? (SOC_GPIO_VALID_GPIO_MASK >> 32) : (uint32_t)SOC_GPIO_VALID_GPIO_MASK;
	if (!pins || (pins != (pins & mask)))
		xsUnknownError("invalid pin");

	if (!builtinArePinsFree(bank, pins))
		xsUnknownError("in use");

	xsmcGet(tmp, xsArg(0), xsID_mode);
	mode = builtinGetSignedInteger(the, &tmp);
	if (!(((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)) ||
		(kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)))
		xsRangeError("invalid mode");

	if ((kDigitalInput <= mode) && (mode <= kDigitalInputPullUpDown)) {
		onReadable = builtinGetCallback(the, xsID_onReadable);
		if (onReadable) {
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
	else if ((kDigitalOutput == mode) || (kDigitalOutputOpenDrain == mode)) {
		mask = bank ? (SOC_GPIO_VALID_OUTPUT_GPIO_MASK >> 32) : (uint32_t)SOC_GPIO_VALID_OUTPUT_GPIO_MASK; 
		if (pins != (pins & mask))
			xsRangeError("input only");
	}
	else
		xsRangeError("invalid mode");

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
// exception for rise/fall on pin 16
		digital->onReadable = onReadable;

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

	if (digital->pins) {
		int pin, lastPin = digital->bank ? GPIO_NUM_MAX - 1 : 31;

		for (pin = digital->bank ? 32 : 0; pin <= lastPin; pin++) {
			if (digital->pins & (1 << (pin & 0x1f))) {
				if (digital->hasOnReadable)
					gpio_isr_handler_remove(pin);
				gpio_reset_pin(pin);
			}
		}

		if (NULL == gDigitals)
			gpio_uninstall_isr_service();
		builtinFreePins(digital->bank, digital->pins);
	}

	builtinCriticalSectionEnd();

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

	if (!digital->isInput)
		xsUnknownError("unimplemented");

	gpio_dev_t *hw = &GPIO;

#if kCPUESP32C3
	result = hw->in.data;
#else
    if (digital->bank)
        result = hw->in1.data;
    else
        result = hw->in;
#endif

	xsmcSetInteger(xsResult, result & digital->pins);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value;

	if (digital->isInput)
		xsUnknownError("unimplemented");

	gpio_dev_t *hw = &GPIO;
	value = xsmcToInteger(xsArg(0)) & digital->pins;

#if kCPUESP32C3
	hw->out_w1ts.out_w1ts = value;
	hw->out_w1tc.out_w1tc = ~value & digital->pins;
#else
	if (digital->bank) {
		hw->out1_w1ts.data = value;
		hw->out1_w1tc.data = ~value & digital->pins;
	}
	else {
		hw->out_w1ts = value;
		hw->out_w1tc = ~value & digital->pins;
	}
#endif
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


//@@ verify read is allowed
uint32_t modDigitalBankRead(Digital digital)
{
	gpio_dev_t *hw = &GPIO;

#if kCPUESP32C3
	return hw->in.data & digital->pins;
#else
    if (digital->bank)
        return hw->in1.data & digital->pins;

	return hw->in & digital->pins;
#endif
}

//@@ verify write is allowed
void modDigitalBankWrite(Digital digital, uint32_t value)
{
	gpio_dev_t *hw = &GPIO;
	value &= digital->pins;

#if kCPUESP32C3
	hw->out_w1ts.out_w1ts = value;
	hw->out_w1tc.out_w1tc = ~value & digital->pins;
#else
	if (digital->bank) {
		hw->out1_w1ts.data = value;
		hw->out1_w1tc.data = ~value & digital->pins;
	}
	else {
		hw->out_w1ts = value;
		hw->out_w1tc = ~value & digital->pins;
	}
#endif
}
