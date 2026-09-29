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

#include "xsHost.h"

#ifdef mxDebug
	extern void fxDisconnect(xsMachine *the);
#endif

void xs_system_restart(xsMachine *the)
{
#ifdef mxDebug
	// tell the debugger the device is going away, as its own restart command does
	fxDisconnect(the);
	modDelayMilliseconds(1000);
#endif

#if defined(__ZEPHYR__)
	sys_reboot(SYS_REBOOT_COLD);
#elif ESP32
	esp_restart();
#elif defined(__ets__)
	system_restart();
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
