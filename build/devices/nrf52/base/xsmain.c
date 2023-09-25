/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "xs.h"
#include "xsmain.h"
#include "modTimer.h"
#include "modInstrumentation.h"

#include "xsPlatform.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef MODDEF_XS_TEST
	#define MODDEF_XS_TEST 1
#endif

xsMachine *gThe = NULL;        // main VM

#ifdef mxDebug
	TaskHandle_t gMainTask = NULL;
#endif

void loop_task(void *pvParameter);

// #define kStack ((10 * 1024) / sizeof(StackType_t))
#define kStack ((6 * 1024) / sizeof(StackType_t))
#define kTaskPriority	6	// (tskIDLE_PRIORITY + 1)

void xs_setup(void)
{
#if configSUPPORT_STATIC_ALLOCATION
	static StaticTask_t taskBuffer;
	static StackType_t stackBuffer[kStack];
	xTaskCreateStatic(loop_task, "main", kStack, NULL, kTaskPriority, stackBuffer, &taskBuffer);
#else
	xTaskCreate(loop_task, "main", kStack, NULL, kTaskPriority, NULL);
#endif
}

void loop_task(void *pvParameter)
{
	taskYIELD();

#ifdef mxDebug
	gMainTask = xTaskGetCurrentTaskHandle();
	setupDebugger();
#endif

	while (true) {
#if MODDEF_XS_TEST
	    gThe = modCloneMachine(NULL, NULL);

		modRunMachineSetup(gThe);

		xsMachine *the = gThe;
		while (gThe) {
#ifdef mxDebug
			uint32_t num = ulTaskNotifyTake(pdTRUE, 0);
			if (num) // got notification from usb driver that there is data available.
				fxReceiveLoop();
#endif
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}

		xsDeleteMachine(the);

#else
		while (true) {
#ifdef mxDebug
			uint32_t num = ulTaskNotifyTake(pdTRUE, 0);
			if (num) // got notification from usb driver that there is data available.
				fxReceiveLoop();
#endif
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
#endif
	}
}

void modLog_transmit(const char *msg)
{
	uint8_t c;

#ifdef mxDebug
	if (gThe) {
		while (0 != (c = c_read8(msg++)))
			fx_putc(gThe, c);
		fx_putc(gThe, 0);
	}
	else
#endif
	{
		while (0 != (c = c_read8(msg++)))
			ESP_putc(c);
		ESP_putc('\r');
		ESP_putc('\n');
	}
}

