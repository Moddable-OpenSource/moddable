/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "xsmain.h"
#include "modTimer.h"

#include "xsPlatform.h"
#include "xsHost.h"
#include "modInstrumentation.h"

xsMachine *gThe = NULL;        // the one XS6 virtual machine running

xsCallback xsHostModuleAt(xsIndex i)
{
    return NULL;
}

extern void mc_setup(xsMachine *the);

void xsTask(void *pvParameter);

#define kStack ((10 * 1024) / sizeof(StackType_t))

void xs_setup() {
	xTaskCreate(xsTask, "main", kStack, NULL, 4, NULL);
	vTaskStartScheduler();
}

void xsTask(void *pvParameter) {

	taskYIELD();
#ifdef mxDebug
	setupDebugger();
#endif

    gThe = ESP_cloneMachine(0, 0, 0, 0);

	mc_setup(gThe);

	xs_start();
}

void xs_loop(void)
{
    if (!gThe)
        return;

    modTimersExecute();
	modMessageService(gThe, modTimersNext());
}

void xs_start() {
	while (1) {
		xs_loop();
	}

}

