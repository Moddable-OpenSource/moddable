/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "xsmc.h"
#include "xsHost.h"
#include "xsHosts.h"
#include "mc.defines.h"

#include "modInstrumentation.h"

xsMachine *gThe = NULL;

#if MODDEF_XS_TEST
	uint8_t gSoftReset;
#endif

static void runLoop(void *p1, void *p2, void *p3)
{
	uint32_t running = 0;
	setupDebugger(&running);

	while (running == 0) {
		modDelayMilliseconds(10);
	}

	while(1) {
		gThe = modCloneMachine(NULL, NULL);
		modRunMachineSetup(gThe);

#if MODDEF_XS_TEST
		xsMachine *the = gThe;
		gSoftReset = 0;
		while (!gSoftReset) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
		xsDeleteMachine(the);
#else
		while (true) {
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
#endif
	}
}

#define PRIORITY 3
K_THREAD_DEFINE(main_thread, 4096, runLoop, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	// nuthin.

	return 0;
}
