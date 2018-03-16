/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#define __XS6PLATFORMMINIMAL__
#include "xs.h"
#include "xsgecko.h"

#include "xsPlatform.h"
#include "modInstrumentation.h"

#include "em_system.h"

xsMachine *gThe = NULL;        // the one XS6 virtual machine running
uint32_t gDeviceUnique;

uint8_t triggerDebugCommand(xsMachine *the);

xsCallback xsHostModuleAt(xsIndex i)
{
    return NULL;
}

extern void mc_setup(xsMachine *the);
extern void geckoSetupGPIO();
extern void geckoSetupRTCC();
extern void	geckoConfigureSysTick();
extern void	setupDebugger();

void xs_setup() {
	gDeviceUnique = SYSTEM_GetUnique() & 0xffffffff;
	geckoSetupGPIO();
	geckoStartRTCC();
	geckoConfigureSysTick();
	setupDebugger();

    gThe = ESP_cloneMachine(0, 0, 0, 0);

	mc_setup(gThe);
}

void xs_loop(void)
{
    if (!gThe)
        return;

#if mxDebug
    if (ESP_isReadable()) {
        if (triggerDebugCommand(gThe)) {
            xsBeginHost(gThe);
            xsDebugger();
            xsEndHost(gThe);
        }
    }
#endif

    modTimersExecute();
	
	if (0 == modMessageService()) {
		int delayMS = modTimersNext();
	    if (delayMS)
    		gecko_delay(delayMS);
	}
}

