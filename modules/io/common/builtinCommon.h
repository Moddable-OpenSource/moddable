/*
 * Copyright (c) 2019-2021 Moddable Tech, Inc.
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

#ifndef __BUILTINCOMMON_H__
#define __BUILTINCOMMON_H__




xsSlot *builtinGetCallback(xsMachine *the, xsIdentifier id);

#define __COMMON__PINS__ 1
#if ESP32
#if kCPUESP32C3
	#define kPinBanks (1)
#else
	#define kPinBanks (2)
#endif

	extern portMUX_TYPE gCommonCriticalMux;
	#define builtinCriticalSectionBegin() portENTER_CRITICAL(&gCommonCriticalMux)
	#define builtinCriticalSectionEnd() portEXIT_CRITICAL(&gCommonCriticalMux)

#elif defined(__ets__)
	#include "Arduino.h"	// mostly to get xs_rsil

	#define kPinBanks (1)

	#define builtinCriticalSectionBegin() xt_rsil(0)
	#define builtinCriticalSectionEnd() xt_rsil(15)
#elif defined(PICO_BUILD)
	#include "pico/critical_section.h"
	#define kPinBanks	(2)

	extern critical_section_t gCommonCriticalMux;
	#define builtinCriticalSectionBegin()	critical_section_enter_blocking(&gCommonCriticalMux)
	#define builtinCriticalSectionEnd()		critical_section_exit(&gCommonCriticalMux)
#else
	#undef __COMMON__PINS__
#endif


enum {
	kIOFormatNumber = 1,
	kIOFormatBuffer = 2,
	kIOFormatStringASCII = 3,
	kIOFormatStringUTF8 = 4,
	kIOFormatSocketTCP = 5,

	kIOFormatNext,
	kIOFormatInvalid = 0xFF,
};

void builtinGetFormat(xsMachine *the, uint8_t format);
uint8_t builtinSetFormat(xsMachine *the);

void builtinInitializeTarget(xsMachine *the);
uint8_t builtinInitializeFormat(xsMachine *the, uint8_t format);

int32_t builtinGetSignedInteger(xsMachine *the, xsSlot *slot);
uint32_t builtinGetUnsignedInteger(xsMachine *the, xsSlot *slot);

#if __COMMON__PINS__
	#define builtinIsPinFree(pin) builtinArePinsFree(pin >> 5, 1 << (pin & 0x1F))
	#define builtinUsePin(pin) builtinUsePins(pin >> 5, 1 << (pin & 0x1F))
	#define builtinFreePin(pin) builtinFreePins(pin >> 5, 1 << (pin & 0x1F))

	uint8_t builtinArePinsFree(uint32_t bank, uint32_t pin);
	uint8_t builtinUsePins(uint32_t bank, uint32_t pin);
	void builtinFreePins(uint32_t bank, uint32_t pin);

	#define builtinGetPin(the, slot) builtinGetUnsignedInteger(the, slot)
#endif

#if defined(PICO_BUILD)
uint8_t builtinInitIO(void);
#endif

#endif
