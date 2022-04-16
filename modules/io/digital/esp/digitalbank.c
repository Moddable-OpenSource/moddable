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

#include "user_interface.h"	// esp8266 functions

#define GPCD   2  // DRIVER 0: normal, 1: open drain

#define GPIO_INIT_OUTPUT(index, opendrain) \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x10) |= (1 << index);					/* enable for write */ \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x28 + (index << 2)) &= ~((opendrain ? 0 : 1) << GPCD);	/* normal (not open-drain) */ \

#define GPIO_INIT_INPUT(index) \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x10) &= ~(1 << index);					/* disable write (e.g. read) */ \
		*(volatile uint32_t *)(PERIPHS_GPIO_BASEADDR + 0x28 + (index << 2)) &= ~(1 << GPCD);	/* normal (not open-drain) */

#define GPIO_CLEAR(index) (GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << index))
#define GPIO_SET(index) (GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << index))

static const uint32_t gPixMuxAddr[] ICACHE_RODATA_ATTR = {
	PERIPHS_IO_MUX_GPIO0_U,
	PERIPHS_IO_MUX_U0TXD_U,
	PERIPHS_IO_MUX_GPIO2_U,
	PERIPHS_IO_MUX_U0RXD_U,
	PERIPHS_IO_MUX_GPIO4_U,
	PERIPHS_IO_MUX_GPIO5_U,
	PERIPHS_IO_MUX_SD_CLK_U,
	PERIPHS_IO_MUX_SD_DATA0_U,
	PERIPHS_IO_MUX_SD_DATA1_U,
	PERIPHS_IO_MUX_SD_DATA2_U,
	PERIPHS_IO_MUX_SD_DATA3_U,
	PERIPHS_IO_MUX_SD_CMD_U,
	PERIPHS_IO_MUX_MTDI_U,
	PERIPHS_IO_MUX_MTCK_U,
	PERIPHS_IO_MUX_MTMS_U,
	PERIPHS_IO_MUX_MTDO_U
};

static const uint8_t gPixMuxValue[] ICACHE_RODATA_ATTR = {
	FUNC_GPIO0,
	FUNC_GPIO1,
	FUNC_GPIO2,
	FUNC_GPIO3,
	FUNC_GPIO4,
	FUNC_GPIO5,
	FUNC_GPIO6,
	FUNC_GPIO7,
	FUNC_GPIO8,
	FUNC_GPIO9,
	FUNC_GPIO10,
	FUNC_GPIO11,
	FUNC_GPIO12,
	FUNC_GPIO13,
	FUNC_GPIO14,
	FUNC_GPIO15
};

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
	// fields after here only allocated if onReadable callback present
	uint16_t	triggered;
	uint16_t	rises;
	uint16_t	falls;

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

static uint8_t gDigitalCallbackPending;

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
	uint8_t bank = 0;
	xsSlot *onReadable;
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

	if (kDigitalInputPullUpDown == mode)
		xsRangeError("invalid mode");		// unavailable

	if ((16 == pin) && ((kDigitalInput != mode) && (kDigitalOutput != mode)))
		xsRangeError("invalid mode");

	digital = c_malloc(onReadable ? sizeof(DigitalRecord) : offsetof(DigitalRecord, triggered));
	if (!digital)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, digital);
	digital->pins = 0;
	digital->obj = xsThis;
	xsRemember(digital->obj);

	for (pin = 0; pin < 17; pin++) {
		if (!(pins & (1 << pin)))
			continue;

		switch (mode) {
			case kDigitalInput:
			case kDigitalInputPullUp:
			case kDigitalInputPullDown:
				if (pin < 16) {
					PIN_FUNC_SELECT(gPixMuxAddr[pin], c_read8(&gPixMuxValue[pin]));
					GPIO_INIT_INPUT(pin);

					if (mode == kDigitalInputPullUp)
						*(volatile uint32_t *)gPixMuxAddr[pin] |= 1 << 7;
					else if (mode == kDigitalInputPullDown)
						*(volatile uint32_t *)gPixMuxAddr[pin] |= 1 << 6;
				}
				else if (16 == pin) {
					WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
								   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

					WRITE_PERI_REG(RTC_GPIO_CONF,
								   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

					WRITE_PERI_REG(RTC_GPIO_ENABLE,
								   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
				}
				break;

			case kDigitalOutput:
			case kDigitalOutputOpenDrain:
				if (pin < 16) {
					PIN_FUNC_SELECT(gPixMuxAddr[pin], c_read8(&gPixMuxValue[pin]));
					GPIO_INIT_OUTPUT(pin, kDigitalOutputOpenDrain == mode);
					GPIO_CLEAR(pin);
				}
				else if (16 == pin) {
					WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
								   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

					WRITE_PERI_REG(RTC_GPIO_CONF,
								   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

					WRITE_PERI_REG(RTC_GPIO_ENABLE,
								   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
				}
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

		builtinCriticalSectionBegin();

			if (NULL == gDigitals) {
				gDigitalCallbackPending = 0;
				ETS_GPIO_INTR_ATTACH(digitalISR, NULL);
				ETS_GPIO_INTR_ENABLE();
			}

			digital->next = gDigitals;
			gDigitals = digital;

			// enable interrupt for these pins
			GPC(pin) &= ~(0xF << GPCI);
			GPIEC = pins & 0xFFFF;
			for (pin = 0; pin < 16; pin++) {
				if (pins & (1 << pin)) {
					uint8_t edge = ((rises & (1 << pin)) ? 1 : 0) | ((falls & (1 << pin)) ? 2 : 0);
					GPC(pin) |= ((edge & 0xF) << GPCI);
				}
			}

		builtinCriticalSectionEnd();
	}

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsDigitalBankHooks);

	digital->pins = pins;
	digital->bank = bank;
	digital->hasOnReadable = onReadable ? 1 : 0;
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
		//@@ what state should pin go to on close...
		uint8_t pin;

		// disable interrupts for these pins
		for (pin = 0; pin < 16; pin++) {
			if (digital->pins & (1 << pin)) {
				GPC(pin) &= ~(0xF << GPCI);
				GPIEC = (1 << pin);
			}
		}

		// remove ISR
		if (NULL == gDigitals)
			ETS_GPIO_INTR_DISABLE();
		builtinFreePins(digital->bank, digital->pins);
	}

	builtinCriticalSectionEnd();

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

	result = GPIO_REG_READ(GPIO_IN_ADDRESS) & digital->pins;
	if (digital->pins & 0x10000) {
		if (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1)
			result |= 0x10000;
		else
			result &= ~0x10000;
	}

	xsmcSetInteger(xsResult, result);
}

void xs_digitalbank_write(xsMachine *the)
{
	Digital digital = xsmcGetHostDataValidate(xsThis, (void *)&xsDigitalBankHooks);
	uint32_t value = xsmcToInteger(xsArg(0)) & digital->pins;

	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, value);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, ~value & digital->pins);
	if (digital->pins & 0x10000)
		WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (value >> 16));
}

void ICACHE_RAM_ATTR digitalISR(void *ignore)
{
	uint32_t status = GPIE;
	GPIEC = status;				// clear interrupts
	if (!status)
		return;

	ETS_GPIO_INTR_DISABLE();
	uint32_t levels = GPI;
	uint8_t doUpdate = 0;
	Digital walker;
	for (walker = gDigitals; walker; walker = walker->next) {
		if (!(walker->pins & status))
			continue;

		if ((levels & walker->rises) || (~levels & walker->falls)) {		// mask rises & falls with status? otherwise may be false trigger?
			walker->triggered |= levels;
			doUpdate = 1;
		}
	}

	ETS_GPIO_INTR_ENABLE();

	if (doUpdate && !gDigitalCallbackPending) {
		gDigitalCallbackPending = 1;
		modMessagePostToMachineFromPool(NULL, digitalDeliver, NULL);		// N.B. no THE required on ESP8266 since it is single threaded... would be unsafe on ESP32
	}
}

void digitalDeliver(void *notThe, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Digital walker;

	gDigitalCallbackPending = 0;

//@@ bad things happen if a digital instance is closed inside this loop
	for (walker = gDigitals; walker; walker = walker->next) {
		uint32_t triggered;

		builtinCriticalSectionBegin();
			triggered = walker->triggered;
			walker->triggered = 0;
		builtinCriticalSectionEnd();

		if (!triggered)
			continue;

		xsBeginHost(walker->the);
			xsmcSetInteger(xsResult, triggered);
			xsCallFunction1(xsReference(walker->onReadable), walker->obj, xsResult);
		xsEndHost(walker->the);
	}
}

//@@ verify read is allowed
uint32_t modDigitalBankRead(Digital digital)
{
	uint32_t result = GPIO_REG_READ(GPIO_IN_ADDRESS) & digital->pins;
	if (digital->pins & 0x10000) {
		if (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1)
			result |= 0x10000;
		else
			result &= ~0x10000;
	}
	return result;
}

//@@ verify write is allowed
void modDigitalBankWrite(Digital digital, uint32_t value)
{
	value &= digital->pins;

	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, value);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, ~value & digital->pins);
	if (digital->pins & 0x10000)
		WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (value >> 16));
}
