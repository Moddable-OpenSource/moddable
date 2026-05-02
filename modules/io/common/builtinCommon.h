/*
 * Copyright (c) 2019-2025 Moddable Tech, Inc.
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

#if ESP32
	#include "freertos/FreeRTOS.h"

	#if kCPUESP32C3 || kCPUESP32C6 || kCPUESP32H2
		#define kPinBanks (1)
	#else
		#define kPinBanks (2)
	#endif

	extern portMUX_TYPE gCommonCriticalMux;
	#define builtinCriticalSectionBegin() portENTER_CRITICAL(&gCommonCriticalMux)
	#define builtinCriticalSectionEnd() portEXIT_CRITICAL(&gCommonCriticalMux)

	#if ESP32
		// IDF invokes abort() when creating socket if no network configured
		#define CHECK_NETWORK_SAFE() \
			if (!esp_netif_get_default_netif()) { \
				xsUnknownError("no network"); \
			}
	#endif
#elif defined(__ets__) && !defined(__ZEPHYR__)
	#include "Arduino.h"	// mostly to get xs_rsil

	#define kPinBanks (1)

	#define builtinCriticalSectionBegin() xt_rsil(0)
	#define builtinCriticalSectionEnd() xt_rsil(15)
#elif nrf52
	#define kPinBanks (2)
	#define GPIO_NUM_MAX (64)

	#define builtinCriticalSectionBegin() vPortEnterCritical()
	#define builtinCriticalSectionEnd() vPortExitCritical()
#elif defined(PICO_BUILD)
	#include "pico/critical_section.h"
	#define kPinBanks	(2)

	extern critical_section_t gCommonCriticalMux;
	#define builtinCriticalSectionBegin()	critical_section_enter_blocking(&gCommonCriticalMux)
	#define builtinCriticalSectionEnd()		critical_section_exit(&gCommonCriticalMux)
#elif defined(__ZEPHYR__)
	#include "mc.devicetree.h"

	#define kPinBanks kModZephyrGPIOBankCount
	#define builtinCriticalSectionBegin()	\
		unsigned int __key = irq_lock();
	#define builtinCriticalSectionEnd() \
		irq_unlock(__key);
#endif

enum {
	kIOFormatNumber = 1,
	kIOFormatBuffer = 2,
	kIOFormatString = 3,
	kIOFormatSocketTCP = 4,

	kIOFormatUint8 = 5,
	kIOFormatInt8 = 6,
	kIOFormatUint16 = 7,
	kIOFormatInt16 = 8,
	kIOFormatUint32 = 9,
	kIOFormatInt32 = 10,
	kIOFormatUint64 = 11,
	kIOFormatInt64 = 12,

	kIOFormatBufferDisposable = 13,

	kIOFormatInvalid = 0xFF,
};


#ifndef CHECK_NETWORK_SAFE
	#define CHECK_NETWORK_SAFE()
#endif

#ifdef __cplusplus
extern "C" {
#endif

void builtinGetFormat(xsMachine *the, uint8_t format);
uint8_t builtinSetFormat(xsMachine *the);

void builtinInitializeTarget(xsMachine *the);
uint8_t builtinInitializeFormat(xsMachine *the, uint8_t format);

int32_t builtinGetSignedInteger(xsMachine *the, xsSlot *slot);
uint32_t builtinGetUnsignedInteger(xsMachine *the, xsSlot *slot);

xsSlot *builtinGetCallback(xsMachine *the, xsIdentifier id);

#ifdef __cplusplus
}
#endif

#ifdef kPinBanks
	#define builtinIsPinFree(pin) builtinArePinsFree(pin >> 5, 1 << (pin & 0x1F))
	#define builtinUsePin(pin) builtinUsePins(pin >> 5, 1 << (pin & 0x1F))
	#define builtinFreePin(pin) builtinFreePins(pin >> 5, 1 << (pin & 0x1F))

	uint8_t builtinArePinsFree(uint32_t bank, uint32_t pin);
	uint8_t builtinUsePins(uint32_t bank, uint32_t pin);
	void builtinFreePins(uint32_t bank, uint32_t pin);

	#define builtinGetPin(the, slot) builtinGetUnsignedInteger(the, slot)
#endif

#if defined(PICO_BUILD) || defined(__ZEPHYR__)
	void builtinInitIO(void);
#else
	#define builtinInitIO()
#endif

#endif
