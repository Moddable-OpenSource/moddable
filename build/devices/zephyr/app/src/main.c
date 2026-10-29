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

int main(void)
{
	// XS runs on the main thread: its stack is CONFIG_MAIN_STACK_SIZE, which mcconfig sets from the manifest's creation.nativeStack
	runLoop(NULL, NULL, NULL);

	return 0;
}

#if !defined(__window_spill)
void __window_spill(void) {}
#endif


