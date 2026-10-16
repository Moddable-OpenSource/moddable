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

#include "xsmc.h"
#include "mc.xs.h"			// for platform xs_ids

#include "xsHost.h"

#if ESP32
	#include "soc/soc_caps.h"
	#if SOC_RTC_WDT_SUPPORTED
		#include "hal/wdt_hal.h"
	#endif
#endif

#ifdef mxDebug
	extern void fxDisconnect(xsMachine *the);
#endif

void xs_system_restart(xsMachine *the)
{
	uint8_t hard = 0;

	if (xsmcArgc && xsmcGet(xsResult, xsArg(0), xsID_type))
		hard = !c_strcmp(xsmcToString(xsResult), "hard");

#ifdef mxDebug
	// tell the debugger the device is going away, as its own restart command does
	fxDisconnect(the);
	modDelayMilliseconds(1000);
#endif

	(void)hard;

#if defined(__ZEPHYR__)
	sys_reboot(hard ? SYS_REBOOT_COLD : SYS_REBOOT_WARM);
#elif ESP32
	#if SOC_RTC_WDT_SUPPORTED
	if (hard) {	// the RTC watchdog's RTC reset resets the whole chip, including the RTC domain, which esp_restart does not
				wdt_hal_context_t rtc_wdt = RWDT_HAL_CONTEXT_DEFAULT();
		wdt_hal_write_protect_disable(&rtc_wdt);
		wdt_hal_config_stage(&rtc_wdt, WDT_STAGE0, 100, WDT_STAGE_ACTION_RESET_RTC);
		wdt_hal_enable(&rtc_wdt);
		wdt_hal_write_protect_enable(&rtc_wdt);
		while (1)		// force watchdog
			;
	}
	#endif
	esp_restart();
#elif defined(ESP8266)
	espRestart();		// boots other slot after successful OTA
#elif defined(PICO_BUILD)
	pico_reset();
#elif nrf52
	nrf52_reset();
#else
	#error "embedded:system restart unsupported"
#endif

	while (1)
		modDelayMilliseconds(1000);
}
