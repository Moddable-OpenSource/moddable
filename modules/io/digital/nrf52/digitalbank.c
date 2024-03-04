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

#include "nrf_drv_gpiote.h"

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

static void digitalISR(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void digitalDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_digitalbank_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static Digital gDigitals;	// pins with onReadable callbacks

static void digitalWakeISR(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

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
	xsSlot *onReadable;
	xsSlot tmp;

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
	digital->pins = 0;
	digital->obj = xsThis;
	digital->useCount = 1;
	xsRemember(digital->obj);

	int lastPin = bank ? GPIO_NUM_MAX - 1 : 31;
	for (pin = bank ? 32 : 0; pin <= lastPin; pin++) {
		if (!(pins & (1 << (pin & 0x1f))))
			continue;

		switch (mode) {
			case kDigitalInput:
	            nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
				break;
			case kDigitalInputPullUp:
	            nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLUP);
				break;
			case kDigitalInputPullDown:
	            nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLDOWN);
				break;
			case kDigitalInputPullUpDown:
				// no nrf52 pullUpDown
	            nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
				break;

			case kDigitalOutput:
				nrf_gpio_cfg_output(pin);
				isInput = 0;
				break;
			case kDigitalOutputOpenDrain:
	            nrf_gpio_cfg(pin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
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

			if (NULL == gDigitals)
				if (!nrf_drv_gpiote_is_init())
					nrf_drv_gpiote_init();

			builtinCriticalSectionBegin();
				digital->next = gDigitals;
				gDigitals = digital;
			builtinCriticalSectionEnd();

			for (pin = bank ? 32 : 0; pin <= lastPin; pin++) {
				uint32_t mask = 1 << (pin & 0x1f);
				if (pins & mask) {
					nrf_drv_gpiote_in_config_t gpiote_config = {0};

					if ((rises & mask) && (falls & mask))
						gpiote_config.sense = NRF_GPIOTE_POLARITY_TOGGLE;
					else if (rises & mask)
						gpiote_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
					else if (falls & mask)
						gpiote_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
					else
					 	gpiote_config.sense = NRF_GPIO_PIN_NOSENSE;
					gpiote_config.pull = nrf_gpio_pin_pull_get(pin);
					gpiote_config.hi_accuracy = true;
					gpiote_config.skip_gpio_setup = true;
					nrf_drv_gpiote_in_init(pin, &gpiote_config, digitalISR);
					nrf_drv_gpiote_in_event_enable(pin, true);
	//				if (NRF_GPIO_PIN_NOSENSE != gpiote_config.sense)
	//					nrf_gpio_cfg_sense_set(pin, gpiote_config.sense);
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
				if (NRF_GPIO_PIN_DIR_INPUT == nrf_gpio_pin_dir_get(pin)) {
					nrf_drv_gpiote_in_event_disable(pin);
					nrf_drv_gpiote_in_uninit(pin);
				}
			}
		}

		if (NULL == gDigitals)
			nrf_drv_gpiote_uninit();

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

	if (!digital->isInput)
		xsUnknownError("can't read output");

	nrf_gpio_ports_read(digital->bank, 1, &result);
	result &= digital->pins;

	xsmcSetInteger(xsResult, result & digital->pins);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value;
	NRF_GPIO_Type *port;

	if (digital->isInput)
		xsUnknownError("can't write input");

	value = xsmcToInteger(xsArg(0)) & digital->pins;

	port = digital->bank ? NRF_P1 : NRF_P0;
	nrf_gpio_port_out_set(port, value);
	nrf_gpio_port_out_clear(port, ~value & digital->pins);
}

void digitalISR(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
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

uint32_t modDigitalBankRead(Digital digital)
{
	uint32_t result;

	nrf_gpio_ports_read(digital->bank, 1, &result);
	return result &= digital->pins;
}

void modDigitalBankWrite(Digital digital, uint32_t value)
{
	NRF_GPIO_Type *port = digital->bank ? NRF_P1 : NRF_P0;

	value &= digital->pins;
	nrf_gpio_port_out_set(port, value);
	nrf_gpio_port_out_clear(port, ~value & digital->pins);
}


