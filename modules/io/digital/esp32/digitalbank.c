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

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"			// esp platform support
#include "mc.xs.h"			// for xsID_* values
#include "digitalbank.h"

#include "builtinCommon.h"

#include "driver/gpio.h"

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
	uint32_t					pins;
	xsSlot						obj;
	uint8_t						bank;
	uint8_t						hasOnReadable;
	uint8_t						isInput;
	uint8_t						useCount;
	// fields after here only allocated if onReadable callback present
	uint32_t					triggered;

	xsMachine					*the;
	xsSlot						*onReadable;
	modDigitalBankOnReadable	onReadableFunc;
	void						*onReadableRefcon;
	struct DigitalRecord *next;
};
typedef struct DigitalRecord DigitalRecord;
typedef struct DigitalRecord *Digital;

static void digitalISR(void *refcon);
static void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_digitalbank_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static void *modDigitalBankValidate(xsMachine *the, xsSlot *instance);
static uint32_t modDigitalBankRead(void *instanceData);
static void modDigitalBankWrite(void *instanceData, uint32_t value);
static uint8_t modDigitalBankSetOnReadable(void *instanceData, modDigitalBankOnReadable func, void *refcon);

static Digital gDigitals;	// pins with onReadable callbacks

static const xsDigitalBankHostHooksRecord ICACHE_RODATA_ATTR xsDigitalBankHooks = {
	.hooks = {
		xs_digitalbank_destructor,
		xs_digitalbank_mark,
		"digitalbank"
	},
	.doValidate = modDigitalBankValidate,
	.doRead = modDigitalBankRead,
	.doWrite = modDigitalBankWrite,
	.doSetOnReadable = modDigitalBankSetOnReadable,
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
#if kPinBanks > 1
	mask = bank ? (SOC_GPIO_VALID_GPIO_MASK >> 32) : (uint32_t)SOC_GPIO_VALID_GPIO_MASK;
#else
	mask = (uint32_t)SOC_GPIO_VALID_GPIO_MASK;
#endif
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
#if kPinBanks > 1
		mask = bank ? (SOC_GPIO_VALID_OUTPUT_GPIO_MASK >> 32) : (uint32_t)SOC_GPIO_VALID_OUTPUT_GPIO_MASK; 
#else
		mask = (uint32_t)SOC_GPIO_VALID_OUTPUT_GPIO_MASK; 
#endif
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
		digital->triggered = 0;
// exception for rise/fall on pin 16
		digital->onReadable = onReadable;
		digital->onReadableFunc = C_NULL;

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

	builtinCriticalSectionEnd();

	if (digital->pins) {
		int pin, lastPin = digital->bank ? GPIO_NUM_MAX - 1 : 31;

		for (pin = digital->bank ? 32 : 0; pin <= lastPin; pin++) {
			if (digital->pins & (1 << (pin & 0x1f))) {
				if (digital->hasOnReadable)
					gpio_isr_handler_remove(pin);
				gpio_reset_pin(pin);
			}
		}

//		if (NULL == gDigitals)
//			gpio_uninstall_isr_service();		// cannot uninstall ISR service as other modules may be relying on it (not reference counted by ESP-IDF)
		builtinFreePins(digital->bank, digital->pins);
	}

#if defined(_NO_ATOMICS)
	if (0 == --digital->useCount)
#else
	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST))
#endif
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

	if (!digital->isInput)
		xsUnknownError("can't read output");

	uint32_t result = modDigitalBankRead(digital);
	xsmcSetInteger(xsResult, result);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);

	if (digital->isInput)
		xsUnknownError("can't write input");

	uint32_t value = xsmcToInteger(xsArg(0));
	modDigitalBankWrite(digital, value);
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

		if (walker->onReadableFunc && (walker->onReadableFunc)(walker->onReadableRefcon))		// all entries in this list have hasOnReadable true
			break;

		uint32_t triggered = walker->triggered;
		walker->triggered |= pin;
		if (!triggered) {
#if defined(_NO_ATOMICS)
			walker->useCount++;
#else
			__atomic_add_fetch(&walker->useCount, 1, __ATOMIC_SEQ_CST);
#endif
			modMessagePostToMachineFromISR(walker->the, digitalDeliver, walker);
		}
		break;
	}
}

void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital digital = refcon;
	uint32_t triggered;

#if defined(_NO_ATOMICS)
	if (0 == --digital->useCount)
#else
	if (0 == __atomic_sub_fetch(&digital->useCount, 1, __ATOMIC_SEQ_CST))
#endif
	{
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

void *modDigitalBankValidate(xsMachine *the, xsSlot *instance)
{
	return xsmcGetHostDataValidate(*instance, (xsHostHooks *)&xsDigitalBankHooks);
}

uint32_t modDigitalBankRead(void *instanceData)
{
	Digital digital = instanceData;
	gpio_dev_t *hw = &GPIO;

	if (!digital->isInput)
		return 0;

#if kCPUESP32C3
	return hw->in.data & digital->pins;
#elif kCPUESP32C6 || kCPUESP32H2
	return hw->in.val & digital->pins;
#else
    if (digital->bank)
        return hw->in1.data & digital->pins;

	return hw->in & digital->pins;
#endif
}

void modDigitalBankWrite(void *instanceData, uint32_t value)
{
	Digital digital = instanceData;
	gpio_dev_t *hw = &GPIO;
	value &= digital->pins;

	if (digital->isInput)
		return;

#if kCPUESP32C3 || kCPUESP32H2
	hw->out_w1ts.out_w1ts = value;
	hw->out_w1tc.out_w1tc = ~value & digital->pins;
#else
	if (digital->bank) {
#if kCPUESP32C6
		hw->out1_w1ts.val = value;
		hw->out1_w1tc.val = ~value & digital->pins;
#else
		hw->out1_w1ts.data = value;
		hw->out1_w1tc.data = ~value & digital->pins;
#endif
	}
	else {
#if kCPUESP32C6
		hw->out_w1ts.val = value;
		hw->out_w1tc.val = ~value & digital->pins;
#else
		hw->out_w1ts = value;
		hw->out_w1tc = ~value & digital->pins;
#endif
	}
#endif
}

uint8_t modDigitalBankSetOnReadable(void *instanceData, modDigitalBankOnReadable func, void *refcon)
{
	Digital digital = instanceData;

	if (!digital->hasOnReadable)
		return 0;

	digital->onReadableFunc = func;
	digital->onReadableRefcon = refcon;

	return 1;
}
